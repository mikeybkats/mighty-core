// GLFW + Dear ImGui debug shell: set BPM, play/stop, flash on each beat from onTick.
// Build target: mighty-core-ui (non-Android only).
// GL_SILENCE_DEPRECATION is set on the target via CMake on Apple.

#include "Metronome.h"
#include "MightyMusicCore.h"
#include "wav_reader.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static constexpr int   WIN_W   = 520;
static constexpr int   WIN_H   = 320;
static constexpr float BPM_MIN = 1.0f;
static constexpr float BPM_MAX = 200.0f;

struct TickSoundUiCtx {
    Metronome* metro = nullptr;
};

static bool tickSoundComboGetter(void* userData, int idx, const char** outText) {
    auto* ctx = static_cast<TickSoundUiCtx*>(userData);
    if (!ctx || !ctx->metro || !outText) {
        return false;
    }
    const int kitN  = ctx->metro->loadedKitSoundCount();
    const int total = 1 + kitN;
    if (idx < 0 || idx >= total) {
        return false;
    }
    if (idx == 0) {
        *outText = "Sine";
        return true;
    }
    const char* label = ctx->metro->loadedKitSoundDisplayName(idx - 1);
    *outText          = label ? label : "(unnamed)";
    return true;
}

static void applyTickSoundCombo(Metronome& metro, int comboIdx) {
    if (comboIdx <= 0) {
        metro.setActiveTickSound(MightyMusicCore::kTickSoundSine);
    } else {
        metro.setActiveTickSound(comboIdx - 1);
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

/// Decodes each sound in the loaded kit (resource basename + `.wav`) from disk.
static bool loadKitWavsFromDisk(Metronome& metro, std::string& statusLine) {
    namespace fs = std::filesystem;
    statusLine.clear();

    const fs::path dir{MIGHTY_CORE_METRONOME_RAW_DIR};
    const int        n = metro.loadedKitSoundCount();
    if (n == 0) {
        statusLine = "Loaded kit has no sounds.";
        return false;
    }

    int ok = 0;
    for (int i = 0; i < n; ++i) {
        const char* base = metro.loadedKitSoundResourceName(i);
        if (!base || !base[0]) {
            break;
        }
        const fs::path path = dir / (std::string(base) + ".wav");
        std::ifstream  f(path, std::ios::binary | std::ios::ate);
        if (!f) {
            statusLine += std::string("Missing: ") + path.string() + "\n";
            continue;
        }
        const auto sz = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::vector<uint8_t> bytes(sz);
        if (!f.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(sz))) {
            statusLine += std::string("Read error: ") + path.string() + "\n";
            continue;
        }

        std::vector<float> pcm;
        int32_t            sampleRate = 0;
        std::string        err;
        if (!decodeWavMono16leToFloat(bytes.data(), bytes.size(), pcm, sampleRate, err)) {
            statusLine += path.filename().string() + ": " + err + "\n";
            continue;
        }

        metro.setTickSoundPcm(i, std::move(pcm), sampleRate);
        ++ok;
    }

    if (ok == n) {
        statusLine = std::string("Kit \"") + (metro.loadedKit() ? metro.loadedKit()->kitId : "?")
            + "\" — loaded " + std::to_string(ok) + " WAV(s) from\n" + dir.string();
        return true;
    }
    if (ok == 0) {
        statusLine += "\nNo kit PCM — tick uses sine for sample slots.";
    } else {
        statusLine += "\nSome kit sounds missing — empty slots use sine.";
    }
    return false;
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

    int   swingIdx = swingComboIndexFromFraction(0.0);
    bool  twoBeat  = false;

    metro.loadKit(Metronome::KitId::MetronomeGentle);
    metro.setBPM(bpm);
    metro.setSwingFraction(kSwingValues[swingIdx]);
    metro.setTwoBeatMeasure(twoBeat);

    TickSoundUiCtx tickUiCtx;
    tickUiCtx.metro = &metro;
    const int        kitSoundCount = metro.loadedKitSoundCount();
    int              tickSoundComboIdx = (kitSoundCount > 0) ? 1 : 0;

    std::string wavLoadStatus;
    loadKitWavsFromDisk(metro, wavLoadStatus);

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

        // ---- Tick sound: sine + one row per entry in the loaded kit definition ----
        ImGui::TextUnformatted("Tick sound");
        ImGui::SetNextItemWidth(-1.0f);
        const int tickComboCount = 1 + metro.loadedKitSoundCount();
        if (ImGui::Combo(
                "##tick_sound",
                &tickSoundComboIdx,
                tickSoundComboGetter,
                &tickUiCtx,
                tickComboCount)) {
            applyTickSoundCombo(metro, tickSoundComboIdx);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.65f, 0.70f, 1.0f));
        ImGui::TextWrapped("%s", wavLoadStatus.c_str());
        ImGui::PopStyleColor();

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
