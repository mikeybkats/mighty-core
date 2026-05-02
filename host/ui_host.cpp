// GLFW + Dear ImGui debug shell: set BPM, play/stop, flash on each beat from onTick.
// Build target: mighty-core-ui (non-Android only).
// GL_SILENCE_DEPRECATION is set on the target via CMake on Apple.

#include "Metronome.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>

static constexpr int   WIN_W   = 520;
static constexpr int   WIN_H   = 320;
static constexpr float BPM_MIN = 1.0f;
static constexpr float BPM_MAX = 200.0f;

static const char* kTickSoundLabels[] = {
    "Sine",
    "PCM slot 0",
    "PCM slot 1",
    "PCM slot 2",
    "PCM slot 3",
};
static constexpr int kTickSoundCount = sizeof(kTickSoundLabels) / sizeof(kTickSoundLabels[0]);

static Metronome::TickSound tickSoundFromComboIndex(int idx) {
    switch (idx) {
        case 1: return Metronome::TickSound::Slot0;
        case 2: return Metronome::TickSound::Slot1;
        case 3: return Metronome::TickSound::Slot2;
        case 4: return Metronome::TickSound::Slot3;
        case 0:
        default: return Metronome::TickSound::Sine;
    }
}

static int comboIndexFromTickSound(Metronome::TickSound s) {
    switch (s) {
        case Metronome::TickSound::Slot0: return 1;
        case Metronome::TickSound::Slot1: return 2;
        case Metronome::TickSound::Slot2: return 3;
        case Metronome::TickSound::Slot3: return 4;
        case Metronome::TickSound::Sine:
        default: return 0;
    }
}

static const char* kSwingLabels[] = {
    "Straight",
    "Light (10%)",
    "Medium (20%)",
    "Heavy (33%)",
    "Max (50%)",
};
static constexpr double kSwingValues[] = {0.0, 0.10, 0.20, 0.33, 0.50};
static constexpr int kSwingPresetCount = sizeof(kSwingValues) / sizeof(kSwingValues[0]);

static int swingComboIndexFromFraction(double f) {
    int best = 0;
    double bestErr = 1e9;
    for (int i = 0; i < kSwingPresetCount; ++i) {
        const double err = std::fabs(f - kSwingValues[i]);
        if (err < bestErr) {
            bestErr = err;
            best = i;
        }
    }
    return best;
}

int main() {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,       GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "Mighty Core — Debug", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io      = ImGui::GetIO();
    io.IniFilename   = nullptr; // don't persist layout to disk

    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0.0f;
    ImGui::GetStyle().FrameRounding  = 4.0f;

    // Scale font for HiDPI / Retina
    float xscale = 1.0f;
    glfwGetWindowContentScale(window, &xscale, nullptr);
    ImFontConfig fontCfg;
    fontCfg.SizePixels    = 15.0f * xscale;
    fontCfg.OversampleH   = 2;
    io.Fonts->AddFontDefault(&fontCfg);
    io.FontGlobalScale    = 1.0f / xscale;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // --- engine (Metronome = policy layer: swing, two-beat, tick sound) ---
    Metronome        metro;
    std::atomic<int> latestBeat{-1};
    metro.onTick = [&](int beat) {
        latestBeat.store(beat, std::memory_order_release);
    };

    float bpm          = 120.0f;
    int   lastBeat     = -1;
    auto  lastBeatTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    int   tickSoundIdx = comboIndexFromTickSound(Metronome::TickSound::Sine);
    int   swingIdx     = swingComboIndexFromFraction(0.0);
    bool  twoBeat     = false;

    metro.setBPM(bpm);
    metro.setTickSound(tickSoundFromComboIndex(tickSoundIdx));
    metro.setSwingFraction(kSwingValues[swingIdx]);
    metro.setTwoBeatMeasure(twoBeat);

    // --- main loop ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Detect new beat from audio thread
        int beat = latestBeat.load(std::memory_order_acquire);
        if (beat != lastBeat && beat >= 0) {
            lastBeat     = beat;
            lastBeatTime = std::chrono::steady_clock::now();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Full-window panel
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##root", nullptr,
            ImGuiWindowFlags_NoTitleBar        |
            ImGuiWindowFlags_NoResize          |
            ImGuiWindowFlags_NoMove            |
            ImGuiWindowFlags_NoScrollbar       |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::Spacing();
        ImGui::Text("Mighty Core  —  Debug Host");
        ImGui::Separator();
        ImGui::Spacing();

        // ---- BPM text input ----
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputFloat("##bpm_text", &bpm, 0.0f, 0.0f, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            bpm = std::clamp(bpm, BPM_MIN, BPM_MAX);
            metro.setBPM(bpm);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("BPM  (Enter to apply)");

        // ---- BPM slider ----
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##bpm_slider", &bpm, BPM_MIN, BPM_MAX, "%.1f")) {
            metro.setBPM(bpm);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Play / Stop button ----
        const bool playing = metro.isPlaying();
        if (playing) {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.70f, 0.12f, 0.12f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.90f, 0.18f, 0.18f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.50f, 0.08f, 0.08f, 1.0f});
            if (ImGui::Button("  Stop  ", {0, 34})) metro.stop();
            ImGui::PopStyleColor(3);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.10f, 0.55f, 0.10f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.14f, 0.75f, 0.14f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.06f, 0.38f, 0.06f, 1.0f});
            if (ImGui::Button("  Play  ", {0, 34})) {
                metro.setBPM(bpm);
                metro.start();
            }
            ImGui::PopStyleColor(3);
        }

        // ---- Beat flash indicator ----
        ImGui::SameLine(0.0f, 20.0f);

        float elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - lastBeatTime).count();
        float alpha = std::max(0.0f, 1.0f - elapsed * 4.0f); // 250 ms fade

        // Draw dot manually so it sits vertically centred beside the button
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float  cy     = cursor.y + 17.0f;
        float  cx     = cursor.x + 14.0f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddCircleFilled({cx, cy}, 13.0f,
            ImGui::ColorConvertFloat4ToU32({1.0f, 0.85f, 0.1f, alpha}));
        dl->AddCircle({cx, cy}, 13.0f,
            ImGui::ColorConvertFloat4ToU32({0.6f, 0.50f, 0.1f, 0.7f}), 0, 1.5f);

        // Beat label to the right of the dot
        ImGui::SetCursorScreenPos({cursor.x + 36.0f, cursor.y + 8.0f});
        if (lastBeat >= 0)
            ImGui::Text("Beat  %d", lastBeat);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Tick sound (sine or PCM slots; empty slots fall back to sine) ----
        ImGui::TextUnformatted("Tick sound");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##tick_sound", &tickSoundIdx, kTickSoundLabels, kTickSoundCount)) {
            metro.setTickSound(tickSoundFromComboIndex(tickSoundIdx));
        }

        ImGui::Spacing();

        // ---- Swing preset (0 .. 0.5) ----
        ImGui::TextUnformatted("Swing");
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::Combo("##swing", &swingIdx, kSwingLabels, kSwingPresetCount)) {
            metro.setSwingFraction(kSwingValues[swingIdx]);
        }

        ImGui::Spacing();

        // ---- Two-beat measure feel ----
        if (ImGui::Checkbox("Two-beat measure", &twoBeat)) {
            metro.setTwoBeatMeasure(twoBeat);
        }

        ImGui::End();

        // ---- Render ----
        ImGui::Render();
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.10f, 0.10f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    metro.stop();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
