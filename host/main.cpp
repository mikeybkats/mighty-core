#include "MightyMusicCore.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    const double bpm     = (argc > 1) ? std::atof(argv[1]) : 120.0;
    const double seconds = (argc > 2) ? std::atof(argv[2]) : 8.0;

    MightyMusicCore core;

    // Audio thread writes beat number; main thread reads and prints.
    std::atomic<int> latestBeat{-1};
    core.onTick = [&](int beat) {
        latestBeat.store(beat, std::memory_order_release);
    };

    core.setBPM(bpm);
    core.start();

    if (!core.isPlaying()) {
        std::cerr << "Failed to open audio device.\n";
        return 1;
    }

    std::cout << "mighty-core-host-check: " << bpm << " BPM for "
              << seconds << "s  (Ctrl-C to stop early)\n\n";

    const auto startTime = std::chrono::steady_clock::now();
    int lastPrinted = -1;

    while (true) {
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed >= std::chrono::duration<double>(seconds)) break;

        int beat = latestBeat.load(std::memory_order_acquire);
        if (beat != lastPrinted) {
            lastPrinted = beat;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            std::cout << "  beat " << std::setw(3) << beat
                      << "  t=" << std::setw(6) << ms << " ms\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    core.stop();

    const double expectedBeats = bpm / 60.0 * seconds;
    std::cout << "\nTotal beats: " << (lastPrinted + 1)
              << "  (expected ~" << static_cast<int>(expectedBeats) << ")\n";
    return 0;
}
