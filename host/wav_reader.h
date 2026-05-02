#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Minimal RIFF WAVE parser: PCM format 1, mono, 16-bit little-endian → float [-1, 1].
bool decodeWavMono16leToFloat(
    const uint8_t* bytes,
    size_t size,
    std::vector<float>& outMonoFloat,
    int32_t& outSampleRate,
    std::string& err);
