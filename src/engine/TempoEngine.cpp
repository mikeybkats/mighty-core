#include "engine/TempoEngine.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace {

constexpr float kTwoBeatBackbeatGain = 0.68f;

}  // namespace

#ifdef __ANDROID__
TempoEngine::TempoEngine() : sound_(synthesizer_) {
}
#else
TempoEngine::TempoEngine() : sound_(synthesizer_), desktop_(std::make_unique<DesktopImpl>()) {
}
#endif

Synthesizer& TempoEngine::synthesizer() {
  return synthesizer_;
}

const Synthesizer& TempoEngine::synthesizer() const {
  return synthesizer_;
}

Sound& TempoEngine::sound() {
  return sound_;
}

const Sound& TempoEngine::sound() const {
  return sound_;
}

void TempoEngine::prepareOutputAudio(int32_t deviceSampleRate) {
  sound_.prepareDeviceRate(deviceSampleRate);
  click_.totalLength = sound_.synthesizedClickLength();
  click_.phase = click_.totalLength;
}

void TempoEngine::setTickSoundPcm(int index, std::vector<float> samples, int32_t sourceSampleRate) {
  sound_.setPcmSlot(index, std::move(samples), sourceSampleRate);
}

void TempoEngine::setTickSound(int index) {
  sound_.setSelection(index);
}

void TempoEngine::setTwoBeatMeasure(bool enabled) {
  scheduler_.setTwoBeatMeasure(enabled);
}

void TempoEngine::setSwingFraction(double fraction) {
  scheduler_.setSwingFraction(fraction);
}

void TempoEngine::processAudio(float* output, int32_t numFrames) {
  std::memset(output, 0, numFrames * sizeof(float));

  // Extend any metronome click that began in the previous callback.
  if (!sound_.isIdle(click_)) {
    sound_.renderPartial(output, 0, numFrames, click_);
  }

  synthesizer_.renderVoices(output, 0, numFrames);

  if (!metronomeRunning_) {
    return;
  }

  AdvanceResult ticks = scheduler_.advance(numFrames);
  for (int i = 0; i < ticks.count; ++i) {
    const int32_t tickFrame = ticks.ticks[i].frameOffset;
    const int beat = ticks.ticks[i].beatNumber;

    const bool softenOffbeat = scheduler_.getTwoBeatMeasure() && (beat % 2 == 1);
    const float pairGainMul =
        (scheduler_.getTwoBeatMeasure() && (beat % 2 == 1)) ? kTwoBeatBackbeatGain : 1.f;

    sound_.triggerAt(output, tickFrame, numFrames, softenOffbeat, pairGainMul, click_);

    if (onTick) onTick(beat);
  }
}

void TempoEngine::startMetronome() {
  metronomeRunning_ = true;
  scheduler_.reset();
  click_.phase = click_.totalLength;
}

void TempoEngine::stopMetronome() {
  metronomeRunning_ = false;
}

bool TempoEngine::isMetronomeRunning() const {
  return metronomeRunning_;
}

void TempoEngine::setBPM(double bpm) {
  scheduler_.setBPM(bpm);
}

double TempoEngine::getBPM() const {
  return scheduler_.getBPM();
}

// ---------------------------------------------------------------------------
// Android / Oboe backend
// ---------------------------------------------------------------------------
#ifdef __ANDROID__

#include <android/log.h>

#include <cinttypes>
#define LOG_TAG "TempoEngine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

TempoEngine::~TempoEngine() {
  close();
}

bool TempoEngine::open() {
  if (stream_ && stream_->getState() == oboe::StreamState::Started) {
    return true;
  }

  oboe::AudioStreamBuilder builder;
  oboe::Result result = builder.setDirection(oboe::Direction::Output)
                            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                            ->setSharingMode(oboe::SharingMode::Exclusive)
                            ->setFormat(oboe::AudioFormat::Float)
                            ->setChannelCount(oboe::ChannelCount::Mono)
                            ->setDataCallback(this)
                            ->openStream(stream_);

  if (result != oboe::Result::OK) {
    LOGD("Failed to open stream: %s", oboe::convertToText(result));
    return false;
  }

  const int32_t sr = stream_->getSampleRate();
  scheduler_.setSampleRate(sr);
  scheduler_.reset();
  prepareOutputAudio(sr);

  result = stream_->start();
  if (result != oboe::Result::OK) {
    LOGD("Failed to start stream: %s", oboe::convertToText(result));
    return false;
  }
  LOGD("Stream opened: %d Hz", sr);
  return true;
}

void TempoEngine::close() {
  if (stream_) {
    stream_->stop();
    stream_->close();
    stream_.reset();
  }
  sound_.clearDeviceBuffers();
}

bool TempoEngine::isRunning() const {
  return stream_ && stream_->getState() == oboe::StreamState::Started;
}

oboe::DataCallbackResult TempoEngine::onAudioReady(oboe::AudioStream*, void* audioData,
                                                   int32_t numFrames) {
  processAudio(static_cast<float*>(audioData), numFrames);
  return oboe::DataCallbackResult::Continue;
}

// ---------------------------------------------------------------------------
// Desktop / miniaudio backend
// ---------------------------------------------------------------------------
#else

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct TempoEngine::DesktopImpl {
  ma_device device;
  bool running = false;
};

static void maDataCallback(ma_device* device, void* output, const void* /*input*/,
                           ma_uint32 frameCount) {
  static_cast<TempoEngine*>(device->pUserData)
      ->processAudio(static_cast<float*>(output), static_cast<int32_t>(frameCount));
}

TempoEngine::~TempoEngine() {
  close();
}

bool TempoEngine::open() {
  if (desktop_->running) {
    return true;
  }

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 1;
  config.sampleRate = 48000;
  config.dataCallback = maDataCallback;
  config.pUserData = this;

  if (ma_device_init(nullptr, &config, &desktop_->device) != MA_SUCCESS) return false;

  const int32_t sr = static_cast<int32_t>(desktop_->device.sampleRate);
  scheduler_.setSampleRate(sr);
  scheduler_.reset();
  prepareOutputAudio(sr);

  if (ma_device_start(&desktop_->device) != MA_SUCCESS) {
    ma_device_uninit(&desktop_->device);
    return false;
  }

  desktop_->running = true;
  return true;
}

void TempoEngine::close() {
  if (desktop_ && desktop_->running) {
    ma_device_uninit(&desktop_->device);
    desktop_->running = false;
  }
  sound_.clearDeviceBuffers();
}

bool TempoEngine::isRunning() const {
  return desktop_ && desktop_->running;
}

#endif  // __ANDROID__
