// Dedicated synth host UI for real-time patch shaping.
// Build target: mighty-core-synth-host (non-Android only).
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <string>

#include "MightyMusicCore.h"
#include "SynthRealtimeParams.h"

namespace {

constexpr int kWinW = 1320;
constexpr int kWinH = 740;

const char* midiNoteName(int midi) {
  static const char* kNames[12] = {"C",  "C#", "D",  "D#", "E", "F",
                                   "F#", "G",  "G#", "A",  "A#", "B"};
  if (midi < 0 || midi > 127) return "--";
  return kNames[midi % 12];
}

bool queueParam(MightyMusicCore& core, SynthRealtimeParamId id, float value) {
  return core.queueSynthParamChange(id, value, true);
}

bool drawKnobLikeSlider(const char* label, float* value, float minValue, float maxValue,
                        const char* fmt = "%.3f") {
  ImGui::PushID(label);
  ImGui::TextUnformatted(label);
  ImGui::SetNextItemWidth(-1.0f);
  const bool changed = ImGui::SliderFloat("##v", value, minValue, maxValue, fmt);
  ImGui::PopID();
  return changed;
}

}  // namespace

int main() {
  if (!glfwInit()) return 1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow* window =
      glfwCreateWindow(kWinW, kWinH, "Mighty Synth Host - Boomstar Style", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.FrameRounding = 6.0f;
  style.WindowRounding = 4.0f;
  style.GrabRounding = 6.0f;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  MightyMusicCore core;
  core.openPlayback();

  int patchIndex = 0;
  int midiNote = 48;
  float velocity = 0.9f;
  bool gate = false;
  bool droneHold = true;
  bool pluckMode = false;

  // Boomstar-like defaults
  float osc1RangeFeet = 8.0f;
  bool osc1Sync = false;
  bool osc1SubOsc = false;
  int osc1Waveform = 0;  // 0 square, 1 triangle, 2 saw
  float osc1PulseWidth = 0.50f;
  float osc1Level = 0.60f;
  float osc2RangeFeet = 8.0f;
  int osc2Waveform = 2;  // saw
  bool osc2EnvMod = false;
  float osc2EnvAmountSemis = 0.0f;
  float osc2Level = 0.40f;
  float mixerNoise = 0.0f;
  float mixerRingMod = 0.0f;
  float cutoffHz = 1600.0f;
  float resonance = 0.22f;
  float drive = 0.40f;
  float filterEnvDepth = 0.35f;
  float filterEnvAttack = 0.005f;
  float filterEnvDecay = 0.25f;
  float filterEnvSustain = 0.0f;
  float filterEnvRelease = 0.20f;
  float glideSec = 0.00f;
  float ampAttack = 0.005f;
  float ampDecay = 0.28f;
  float ampSustain = 0.65f;
  float ampRelease = 0.24f;
  float osc1LfoRate = 2.7f;
  float osc1LfoDepth = 0.0f;
  int osc1LfoTarget = 0;  // 0 pitch, 1 pulse width
  float osc2LfoRate = 4.5f;
  float osc2LfoDepth = 0.0f;
  int osc2LfoTarget = 0;
  float masterVolume = 0.70f;
  float fxWet = 0.08f;
  float fxDelayMs = 80.0f;
  float fxDelayFeedback = 0.18f;
  float fxDelayMix = 0.35f;
  float pluckBrightness = 0.58f;
  float pluckDamping = 0.52f;
  float pluckStructure = 0.14f;
  float pluckAccent = 0.92f;

  auto applyPanelParams = [&]() {
    queueParam(core, SynthRealtimeParamId::Osc1RangeFeet, osc1RangeFeet);
    queueParam(core, SynthRealtimeParamId::Osc1Sync, osc1Sync ? 1.0f : 0.0f);
    queueParam(core, SynthRealtimeParamId::Osc1SubOsc, osc1SubOsc ? 1.0f : 0.0f);
    queueParam(core, SynthRealtimeParamId::Osc1Waveform, static_cast<float>(osc1Waveform));
    queueParam(core, SynthRealtimeParamId::Osc1PulseWidth, osc1PulseWidth);
    queueParam(core, SynthRealtimeParamId::Osc1Level, osc1Level);
    queueParam(core, SynthRealtimeParamId::Osc2RangeFeet, osc2RangeFeet);
    queueParam(core, SynthRealtimeParamId::Osc2Waveform, static_cast<float>(osc2Waveform));
    queueParam(core, SynthRealtimeParamId::Osc2EnvMod, osc2EnvMod ? 1.0f : 0.0f);
    queueParam(core, SynthRealtimeParamId::Osc2EnvAmountSemis, osc2EnvAmountSemis);
    queueParam(core, SynthRealtimeParamId::Osc2Level, osc2Level);
    queueParam(core, SynthRealtimeParamId::MixerNoise, mixerNoise);
    queueParam(core, SynthRealtimeParamId::MixerRingMod, mixerRingMod);
    queueParam(core, SynthRealtimeParamId::PluckMode, pluckMode ? 1.0f : 0.0f);
    queueParam(core, SynthRealtimeParamId::FilterCutoffHz, cutoffHz);
    queueParam(core, SynthRealtimeParamId::FilterResonance, resonance);
    queueParam(core, SynthRealtimeParamId::FilterDrive, drive);
    queueParam(core, SynthRealtimeParamId::FilterEnvDepth, filterEnvDepth);
    queueParam(core, SynthRealtimeParamId::FilterEnvAttackSec, filterEnvAttack);
    queueParam(core, SynthRealtimeParamId::FilterEnvDecaySec, filterEnvDecay);
    queueParam(core, SynthRealtimeParamId::FilterEnvSustain, filterEnvSustain);
    queueParam(core, SynthRealtimeParamId::FilterEnvReleaseSec, filterEnvRelease);
    queueParam(core, SynthRealtimeParamId::GlideSec, glideSec);
    queueParam(core, SynthRealtimeParamId::AmpAttackSec, ampAttack);
    queueParam(core, SynthRealtimeParamId::AmpDecaySec, ampDecay);
    queueParam(core, SynthRealtimeParamId::AmpSustain, ampSustain);
    queueParam(core, SynthRealtimeParamId::AmpReleaseSec, ampRelease);
    queueParam(core, SynthRealtimeParamId::MasterVolume, masterVolume);
    queueParam(core, SynthRealtimeParamId::Osc1LfoRate, osc1LfoRate);
    queueParam(core, SynthRealtimeParamId::Osc1LfoDepth, osc1LfoDepth);
    queueParam(core, SynthRealtimeParamId::Osc1LfoTarget, static_cast<float>(osc1LfoTarget));
    queueParam(core, SynthRealtimeParamId::Osc2LfoRate, osc2LfoRate);
    queueParam(core, SynthRealtimeParamId::Osc2LfoDepth, osc2LfoDepth);
    queueParam(core, SynthRealtimeParamId::Osc2LfoTarget, static_cast<float>(osc2LfoTarget));
    queueParam(core, SynthRealtimeParamId::PluckBrightness, pluckBrightness);
    queueParam(core, SynthRealtimeParamId::PluckDamping, pluckDamping);
    queueParam(core, SynthRealtimeParamId::PluckStructure, pluckStructure);
    queueParam(core, SynthRealtimeParamId::PluckAccent, pluckAccent);
    queueParam(core, SynthRealtimeParamId::EffectsWet, fxWet);
    queueParam(core, SynthRealtimeParamId::EffectsDelayMs, fxDelayMs);
    queueParam(core, SynthRealtimeParamId::EffectsDelayFeedback, fxDelayFeedback);
    queueParam(core, SynthRealtimeParamId::EffectsDelayMix, fxDelayMix);
  };

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##synth_root", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Text("Mighty Synth Host");
    ImGui::Separator();
    ImGui::Spacing();

    bool controlsChanged = false;

    ImGui::BeginChild("transport", ImVec2(0, 120), true);
    ImGui::TextUnformatted("Voice + Gate");
    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::BeginCombo("Patch", core.synthPatchName(patchIndex))) {
      for (int i = 0; i < MightyMusicCore::kSynthPatchCount; ++i) {
        const bool selected = (i == patchIndex);
        if (ImGui::Selectable(core.synthPatchName(i), selected)) {
          patchIndex = i;
          controlsChanged = true;
          if (gate) {
            core.triggerSynthNote(patchIndex, midiNote, velocity);
            applyPanelParams();
          }
        }
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    const int previousMidi = midiNote;
    controlsChanged |= ImGui::SliderInt("Pitch", &midiNote, 24, 96);
    ImGui::SameLine();
    ImGui::Text("%s%d", midiNoteName(midiNote), (midiNote / 12) - 1);

    ImGui::SetNextItemWidth(220.0f);
    controlsChanged |= ImGui::SliderFloat("Velocity", &velocity, 0.1f, 1.0f, "%.2f");
    ImGui::SameLine();
    if (ImGui::Checkbox("Gate", &gate)) {
      controlsChanged = true;
      if (gate) {
        core.triggerSynthNote(patchIndex, midiNote, velocity);
        applyPanelParams();
      } else {
        core.releaseSynthGate();
      }
    }
    ImGui::SameLine();
    controlsChanged |= ImGui::Checkbox("Drone hold", &droneHold);
    ImGui::SameLine();
    if (ImGui::Checkbox("Pluck mode", &pluckMode)) {
      controlsChanged = true;
      applyPanelParams();
    }

    if (gate && previousMidi != midiNote) {
      if (droneHold) {
        core.setSynthPitch(midiNote);
      } else {
        core.triggerSynthNote(patchIndex, midiNote, velocity);
      }
      applyPanelParams();
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::TextUnformatted("Boomstar-style control panel");
    ImGui::Separator();

    const char* waveItems = "Square\0Triangle\0Saw\0";
    const char* feetItems = "32\0""16\0""8\0""4\0""2\0";

    ImGui::Columns(7, "synth_cols", false);

    ImGui::BeginDisabled(pluckMode);
    ImGui::BeginChild("col_osc1", ImVec2(0, 430), true);
    ImGui::TextUnformatted("Osc 1");
    ImGui::SetNextItemWidth(-1.0f);
    int osc1FeetIdx = (osc1RangeFeet >= 28.f) ? 0 : (osc1RangeFeet >= 12.f) ? 1
                                       : (osc1RangeFeet >= 6.f)  ? 2
                                       : (osc1RangeFeet >= 3.f)  ? 3
                                                                  : 4;
    if (ImGui::Combo("Range (feet)", &osc1FeetIdx, feetItems)) {
      const float map[5] = {32.f, 16.f, 8.f, 4.f, 2.f};
      osc1RangeFeet = map[osc1FeetIdx];
      controlsChanged = true;
    }
    ImGui::SetNextItemWidth(-1.0f);
    controlsChanged |= ImGui::Combo("Waveform", &osc1Waveform, waveItems);
    controlsChanged |= drawKnobLikeSlider("Width", &osc1PulseWidth, 0.05f, 0.95f, "%.2f");
    controlsChanged |= ImGui::Checkbox("Sync", &osc1Sync);
    controlsChanged |= ImGui::Checkbox("Sub osc", &osc1SubOsc);
    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::NextColumn();

    ImGui::BeginDisabled(pluckMode);
    ImGui::BeginChild("col_osc2", ImVec2(0, 430), true);
    ImGui::TextUnformatted("Osc 2");
    ImGui::SetNextItemWidth(-1.0f);
    int osc2FeetIdx = (osc2RangeFeet >= 28.f) ? 0 : (osc2RangeFeet >= 12.f) ? 1
                                       : (osc2RangeFeet >= 6.f)  ? 2
                                       : (osc2RangeFeet >= 3.f)  ? 3
                                                                  : 4;
    if (ImGui::Combo("Range (feet)", &osc2FeetIdx, feetItems)) {
      const float map[5] = {32.f, 16.f, 8.f, 4.f, 2.f};
      osc2RangeFeet = map[osc2FeetIdx];
      controlsChanged = true;
    }
    ImGui::SetNextItemWidth(-1.0f);
    controlsChanged |= ImGui::Combo("Waveform", &osc2Waveform, waveItems);
    controlsChanged |= ImGui::Checkbox("Amp env mod", &osc2EnvMod);
    controlsChanged |= drawKnobLikeSlider("Env amount", &osc2EnvAmountSemis, -24.0f, 24.0f, "%.1f st");
    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::NextColumn();

    ImGui::BeginDisabled(pluckMode);
    ImGui::BeginChild("col_mixer", ImVec2(0, 430), true);
    ImGui::TextUnformatted("Mixer");
    controlsChanged |= drawKnobLikeSlider("Osc 1", &osc1Level, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Osc 2", &osc2Level, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Noise", &mixerNoise, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Ring mod", &mixerRingMod, 0.0f, 1.0f, "%.2f");
    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::NextColumn();

    ImGui::BeginDisabled(pluckMode);
    ImGui::BeginChild("col_filter", ImVec2(0, 430), true);
    ImGui::TextUnformatted("Filter");
    controlsChanged |= drawKnobLikeSlider("Cutoff", &cutoffHz, 40.0f, 16000.0f, "%.0f Hz");
    controlsChanged |= drawKnobLikeSlider("Resonance", &resonance, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Drive", &drive, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Env depth", &filterEnvDepth, 0.0f, 1.0f, "%.2f");
    ImGui::Separator();
    ImGui::TextUnformatted("Filter env");
    controlsChanged |= drawKnobLikeSlider("Attack##fenv", &filterEnvAttack, 0.0f, 1.2f, "%.3f s");
    controlsChanged |= drawKnobLikeSlider("Decay##fenv", &filterEnvDecay, 0.0f, 2.0f, "%.3f s");
    controlsChanged |= drawKnobLikeSlider("Sustain##fenv", &filterEnvSustain, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Release##fenv", &filterEnvRelease, 0.0f, 2.5f, "%.3f s");
    controlsChanged |= drawKnobLikeSlider("Glide", &glideSec, 0.0f, 0.7f, "%.3f s");
    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::NextColumn();

    ImGui::BeginChild("col_envelope", ImVec2(0, 430), true);
    ImGui::TextUnformatted("Amp env (osc level)");
    controlsChanged |= drawKnobLikeSlider("Attack##aenv", &ampAttack, 0.0f, 1.2f, "%.3f s");
    controlsChanged |= drawKnobLikeSlider("Decay##aenv", &ampDecay, 0.0f, 2.0f, "%.3f s");
    controlsChanged |= drawKnobLikeSlider("Sustain##aenv", &ampSustain, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Release##aenv", &ampRelease, 0.0f, 2.5f, "%.3f s");
    controlsChanged |= drawKnobLikeSlider("Master", &masterVolume, 0.0f, 1.0f, "%.2f");
    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::BeginChild("col_lfo_or_pluck", ImVec2(0, 430), true);
    if (pluckMode) {
      ImGui::TextUnformatted("Pluck");
      controlsChanged |= drawKnobLikeSlider("Brightness", &pluckBrightness, 0.0f, 1.0f, "%.2f");
      controlsChanged |= drawKnobLikeSlider("Damping", &pluckDamping, 0.0f, 1.0f, "%.2f");
      controlsChanged |= drawKnobLikeSlider("Structure", &pluckStructure, 0.0f, 1.0f, "%.2f");
      controlsChanged |= drawKnobLikeSlider("Accent", &pluckAccent, 0.0f, 1.0f, "%.2f");
    } else {
      const char* lfoTargetItems = "Pitch\0Pulse width\0";
      ImGui::TextUnformatted("LFO 1 (Osc 1)");
      ImGui::SetNextItemWidth(-1.0f);
      controlsChanged |= ImGui::Combo("Target##lfo1", &osc1LfoTarget, lfoTargetItems);
      controlsChanged |= drawKnobLikeSlider("Rate##lfo1", &osc1LfoRate, 0.02f, 20.0f, "%.2f Hz");
      controlsChanged |= drawKnobLikeSlider("Depth##lfo1", &osc1LfoDepth, 0.0f, 1.0f, "%.2f");
      ImGui::Separator();
      ImGui::TextUnformatted("LFO 2 (Osc 2)");
      ImGui::SetNextItemWidth(-1.0f);
      controlsChanged |= ImGui::Combo("Target##lfo2", &osc2LfoTarget, lfoTargetItems);
      controlsChanged |= drawKnobLikeSlider("Rate##lfo2", &osc2LfoRate, 0.02f, 20.0f, "%.2f Hz");
      controlsChanged |= drawKnobLikeSlider("Depth##lfo2", &osc2LfoDepth, 0.0f, 1.0f, "%.2f");
    }
    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::BeginChild("col_fx", ImVec2(0, 430), true);
    ImGui::TextUnformatted("FX");
    controlsChanged |= drawKnobLikeSlider("Wet", &fxWet, 0.0f, 1.0f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Delay MS", &fxDelayMs, 1.0f, 700.0f, "%.1f");
    controlsChanged |= drawKnobLikeSlider("Delay FB", &fxDelayFeedback, 0.0f, 0.92f, "%.2f");
    controlsChanged |= drawKnobLikeSlider("Delay Mix", &fxDelayMix, 0.0f, 1.0f, "%.2f");
    if (ImGui::Button("Panic / Gate Off", ImVec2(-1, 36))) {
      controlsChanged = true;
      gate = false;
      core.releaseSynthGate();
    }
    ImGui::EndChild();

    if (controlsChanged) {
      applyPanelParams();
    }

    ImGui::Columns(1);
    ImGui::End();

    ImGui::Render();
    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.07f, 0.07f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  core.releaseSynthGate();
  core.closePlayback();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
