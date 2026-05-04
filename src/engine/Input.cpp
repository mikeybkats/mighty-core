#include "engine/Input.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#ifdef __ANDROID__
#include <oboe/Oboe.h>
#else
#include "miniaudio.h"
#endif

namespace {

constexpr int32_t kInputSampleRate = 48000;
constexpr int32_t kWindowSize = 2048;
constexpr int32_t kHopSize = 512;
constexpr double kMinDetectHz = 41.0;    // Bass E1 and above.
constexpr double kMaxDetectHz = 1046.5;  // C6 upper bound for current targets.
constexpr double kMinRmsGate = 0.01;
constexpr double kMinConfidence = 0.65;

struct DetectionResult {
  bool valid = false;
  double frequencyHz = 0.0;
  int midi = 0;
  double centsOff = 0.0;
  double confidence = 0.0;
};

struct NoteEvent {
  double timestampSec = 0.0;
  double frequencyHz = 0.0;
  int midi = 0;
  double centsOff = 0.0;
  double confidence = 0.0;
};

// Computes RMS (Root Mean Square) amplitude for a sample window.
// We use this as a signal-energy gate so silence/background noise is ignored.
double computeRms(const std::vector<float>& samples, size_t offset, int32_t count) {
  if (count <= 0) {
    return 0.0;
  }

  double sum = 0.0;
  for (int32_t i = 0; i < count; ++i) {
    const double s = samples[offset + static_cast<size_t>(i)];
    sum += s * s;
  }
  return std::sqrt(sum / static_cast<double>(count));
}

// Detects one monophonic pitch from the current window and converts it to:
// - frequency in Hz (Hertz)
// - MIDI note number (Musical Instrument Digital Interface pitch index)
// - cents offset from equal-tempered tuning.
DetectionResult detectPitchMonophonic(const std::vector<float>& samples, size_t offset,
                                      int32_t sampleRate) {
  DetectionResult result;
  if (sampleRate <= 0) {
    return result;
  }

  const int32_t minLag =
      static_cast<int32_t>(std::floor(static_cast<double>(sampleRate) / kMaxDetectHz));
  const int32_t maxLag =
      static_cast<int32_t>(std::ceil(static_cast<double>(sampleRate) / kMinDetectHz));
  if (minLag < 2 || maxLag <= minLag || maxLag >= kWindowSize - 2) {
    return result;
  }

  const double rms = computeRms(samples, offset, kWindowSize);
  if (rms < kMinRmsGate) {
    return result;
  }

  // Normalized autocorrelation score for each lag candidate.
  // "lag" is the sample delay of one waveform period.
  double bestCorr = 0.0;
  int32_t bestLag = minLag;
  for (int32_t lag = minLag; lag <= maxLag; ++lag) {
    double ac = 0.0;
    double e0 = 0.0;
    double e1 = 0.0;
    for (int32_t i = 0; i < kWindowSize - lag; ++i) {
      const double x0 = samples[offset + static_cast<size_t>(i)];
      const double x1 = samples[offset + static_cast<size_t>(i + lag)];
      ac += x0 * x1;
      e0 += x0 * x0;
      e1 += x1 * x1;
    }

    const double norm = std::sqrt(e0 * e1);
    if (norm <= std::numeric_limits<double>::epsilon()) {
      continue;
    }
    const double corr = ac / norm;
    if (corr > bestCorr) {
      bestCorr = corr;
      bestLag = lag;
    }
  }

  if (bestCorr < kMinConfidence) {
    return result;
  }

  double refinedLag = static_cast<double>(bestLag);
  // Quadratic interpolation around the best lag improves frequency precision.
  if (bestLag > minLag && bestLag < maxLag) {
    auto normCorrAtLag = [&](int32_t lag) -> double {
      double ac = 0.0;
      double e0 = 0.0;
      double e1 = 0.0;
      for (int32_t i = 0; i < kWindowSize - lag; ++i) {
        const double x0 = samples[offset + static_cast<size_t>(i)];
        const double x1 = samples[offset + static_cast<size_t>(i + lag)];
        ac += x0 * x1;
        e0 += x0 * x0;
        e1 += x1 * x1;
      }
      const double norm = std::sqrt(e0 * e1);
      if (norm <= std::numeric_limits<double>::epsilon()) {
        return 0.0;
      }
      return ac / norm;
    };

    const double c0 = normCorrAtLag(bestLag - 1);
    const double c1 = normCorrAtLag(bestLag);
    const double c2 = normCorrAtLag(bestLag + 1);
    const double denom = (c0 - (2.0 * c1) + c2);
    if (std::fabs(denom) > 1e-9) {
      const double delta = 0.5 * (c0 - c2) / denom;
      if (std::fabs(delta) < 1.0) {
        refinedLag += delta;
      }
    }
  }

  if (refinedLag <= 0.0) {
    return result;
  }

  const double frequencyHz = static_cast<double>(sampleRate) / refinedLag;
  if (frequencyHz < kMinDetectHz || frequencyHz > kMaxDetectHz) {
    return result;
  }

  const double midiFloat = 69.0 + (12.0 * std::log2(frequencyHz / 440.0));
  const int midi = static_cast<int>(std::lround(midiFloat));
  const double tunedHz = 440.0 * std::pow(2.0, (static_cast<double>(midi) - 69.0) / 12.0);
  const double centsOff = 1200.0 * std::log2(frequencyHz / tunedHz);

  result.valid = true;
  result.frequencyHz = frequencyHz;
  result.midi = midi;
  result.centsOff = centsOff;
  result.confidence = bestCorr;
  return result;
}

// Creates a timestamped CSV path under ./listening_logs for this session.
std::string buildSessionLogPath() {
  namespace fs = std::filesystem;
  const fs::path logDir = fs::current_path() / "listening_logs";
  std::error_code ec;
  fs::create_directories(logDir, ec);

  const auto now = std::chrono::system_clock::now();
  const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
  std::tm nowTm{};
#if defined(_WIN32)
  localtime_s(&nowTm, &nowTime);
#else
  localtime_r(&nowTime, &nowTm);
#endif

  std::ostringstream name;
  name << "session_" << std::put_time(&nowTm, "%Y%m%d_%H%M%S") << ".csv";
  return (logDir / name.str()).string();
}

// Writes all captured note events to CSV for offline evaluation and testing.
void writeSessionLog(const std::string& outputPath, const std::vector<NoteEvent>& events) {
  std::ofstream out(outputPath, std::ios::trunc);
  if (!out.is_open()) {
    return;
  }

  out << "timestamp_sec,midi,frequency_hz,cents_off,confidence\n";
  out << std::fixed << std::setprecision(6);
  for (const auto& event : events) {
    out << event.timestampSec << "," << event.midi << "," << event.frequencyHz << ","
        << event.centsOff << "," << event.confidence << "\n";
  }
}

}  // namespace

class Input::Impl {
 public:
  // Shared listening session state.
  std::atomic<bool> listening{false};
  int32_t inputSampleRate = kInputSampleRate;
  std::mutex stateMutex;
  std::vector<float> inputBuffer;
  std::vector<NoteEvent> capturedNotes;
  int lastMidi = -1;
  std::chrono::steady_clock::time_point listenStartedAt{};
  std::string lastSessionLogPath;

#ifdef __ANDROID__
  // Android input callback forwards microphone buffers into the detector.
  class InputCallback : public oboe::AudioStreamDataCallback {
   public:
    explicit InputCallback(Impl* owner) : owner_(owner) {
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream*, void* audioData,
                                          int32_t numFrames) override {
      owner_->consumeInput(static_cast<const float*>(audioData), numFrames);
      return oboe::DataCallbackResult::Continue;
    }

   private:
    Impl* owner_;
  };

  std::shared_ptr<oboe::AudioStream> inputStream;
  std::unique_ptr<InputCallback> inputCallback;
#else
  // Desktop/mobile-host capture state using miniaudio.
  struct DesktopInput {
    ma_device device;
    bool running = false;
  };
  std::unique_ptr<DesktopInput> desktopInput{std::make_unique<DesktopInput>()};
#endif

  // Accepts mic frames, runs windowed pitch detection, and stores note-change events.
  void consumeInput(const float* input, int32_t numFrames) {
    if (!listening.load(std::memory_order_relaxed) || !input || numFrames <= 0) {
      return;
    }

    std::lock_guard<std::mutex> lock(stateMutex);
    inputBuffer.insert(inputBuffer.end(), input, input + numFrames);

    while (static_cast<int32_t>(inputBuffer.size()) >= kWindowSize) {
      const DetectionResult detection = detectPitchMonophonic(inputBuffer, 0, inputSampleRate);
      if (detection.valid && detection.midi != lastMidi) {
        const auto now = std::chrono::steady_clock::now();
        const double ts = std::chrono::duration<double>(now - listenStartedAt).count();

        capturedNotes.push_back(NoteEvent{
            ts,
            detection.frequencyHz,
            detection.midi,
            detection.centsOff,
            detection.confidence,
        });
        lastMidi = detection.midi;
      }

      // Hop-size stepping keeps latency low while still using a stable analysis window.
      inputBuffer.erase(inputBuffer.begin(),
                        inputBuffer.begin() +
                            std::min<int32_t>(kHopSize, static_cast<int32_t>(inputBuffer.size())));
    }
  }
};

Input::Input() : impl_(std::make_unique<Impl>()) {
}
Input::~Input() {
  stopListening();
}

void Input::startListening() {
  if (impl_->listening.load(std::memory_order_relaxed)) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(impl_->stateMutex);
    impl_->inputBuffer.clear();
    impl_->capturedNotes.clear();
    impl_->lastMidi = -1;
    impl_->listenStartedAt = std::chrono::steady_clock::now();
  }

#ifdef __ANDROID__
  // Android path: Oboe low-latency input stream.
  impl_->inputCallback = std::make_unique<Impl::InputCallback>(impl_.get());

  oboe::AudioStreamBuilder builder;
  const oboe::Result openResult = builder.setDirection(oboe::Direction::Input)
                                      ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                                      ->setSharingMode(oboe::SharingMode::Exclusive)
                                      ->setFormat(oboe::AudioFormat::Float)
                                      ->setChannelCount(oboe::ChannelCount::Mono)
                                      ->setSampleRate(kInputSampleRate)
                                      ->setDataCallback(impl_->inputCallback.get())
                                      ->openStream(impl_->inputStream);

  if (openResult != oboe::Result::OK || !impl_->inputStream) {
    impl_->inputCallback.reset();
    return;
  }

  impl_->inputSampleRate = impl_->inputStream->getSampleRate();
  if (impl_->inputStream->requestStart() != oboe::Result::OK) {
    impl_->inputStream->close();
    impl_->inputStream.reset();
    impl_->inputCallback.reset();
    return;
  }
#else
  // Non-Android path: miniaudio capture stream.
  auto* desktop = impl_->desktopInput.get();
  if (!desktop) {
    return;
  }

  ma_device_config config = ma_device_config_init(ma_device_type_capture);
  config.capture.format = ma_format_f32;
  config.capture.channels = 1;
  config.sampleRate = kInputSampleRate;
  config.pUserData = impl_.get();
  config.dataCallback = [](ma_device* device, void* /*output*/, const void* input,
                           ma_uint32 frameCount) {
    auto* owner = static_cast<Impl*>(device->pUserData);
    owner->consumeInput(static_cast<const float*>(input), static_cast<int32_t>(frameCount));
  };

  if (ma_device_init(nullptr, &config, &desktop->device) != MA_SUCCESS) {
    return;
  }
  impl_->inputSampleRate = static_cast<int32_t>(desktop->device.sampleRate);
  if (ma_device_start(&desktop->device) != MA_SUCCESS) {
    ma_device_uninit(&desktop->device);
    return;
  }
  desktop->running = true;
#endif

  impl_->listening.store(true, std::memory_order_relaxed);
}

void Input::stopListening() {
  if (!impl_->listening.load(std::memory_order_relaxed)) {
    return;
  }

  // Mark inactive first so callback work exits as quickly as possible.
  impl_->listening.store(false, std::memory_order_relaxed);

#ifdef __ANDROID__
  if (impl_->inputStream) {
    impl_->inputStream->requestStop();
    impl_->inputStream->close();
    impl_->inputStream.reset();
  }
  impl_->inputCallback.reset();
#else
  auto* desktop = impl_->desktopInput.get();
  if (desktop && desktop->running) {
    ma_device_uninit(&desktop->device);
    desktop->running = false;
  }
#endif

  std::vector<NoteEvent> sessionNotes;
  {
    std::lock_guard<std::mutex> lock(impl_->stateMutex);
    sessionNotes = impl_->capturedNotes;
    impl_->inputBuffer.clear();
    impl_->capturedNotes.clear();
    impl_->lastMidi = -1;
  }

  impl_->lastSessionLogPath = buildSessionLogPath();
  writeSessionLog(impl_->lastSessionLogPath, sessionNotes);
}
