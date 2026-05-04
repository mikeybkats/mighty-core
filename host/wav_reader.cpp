#include "wav_reader.h"

#include <cstring>

namespace {

uint16_t readLe16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t readLe32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

}  // namespace

bool decodeWavMono16leToFloat(const uint8_t* bytes, size_t size, std::vector<float>& outMonoFloat,
                              int32_t& outSampleRate, std::string& err) {
  err.clear();
  outMonoFloat.clear();
  outSampleRate = 0;

  if (size < 12) {
    err = "file too small";
    return false;
  }
  if (std::memcmp(bytes, "RIFF", 4) != 0 || std::memcmp(bytes + 8, "WAVE", 4) != 0) {
    err = "not a RIFF/WAVE file";
    return false;
  }

  size_t off = 12;
  bool haveFmt = false;
  bool haveData = false;
  int32_t sampleRate = 0;
  uint16_t channels = 0;
  uint16_t bits = 0;
  uint16_t audioFormat = 0;
  const uint8_t* dataPtr = nullptr;
  uint32_t dataSize = 0;

  while (off + 8 <= size) {
    const uint8_t* id = bytes + off;
    const uint32_t chunkSize = readLe32(bytes + off + 4);
    off += 8;
    if (off + chunkSize > size) {
      err = "chunk extends past end of file";
      return false;
    }

    if (std::memcmp(id, "fmt ", 4) == 0) {
      if (chunkSize < 16) {
        err = "fmt chunk too small";
        return false;
      }
      const uint8_t* f = bytes + off;
      audioFormat = readLe16(f + 0);
      channels = readLe16(f + 2);
      sampleRate = static_cast<int32_t>(readLe32(f + 4));
      bits = readLe16(f + 14);
      haveFmt = true;
    } else if (std::memcmp(id, "data", 4) == 0) {
      dataPtr = bytes + off;
      dataSize = chunkSize;
      haveData = true;
    }

    off += chunkSize;
    if ((chunkSize & 1u) != 0) ++off;  // RIFF pad byte for even alignment
  }

  if (!haveFmt || !haveData) {
    err = "missing fmt or data chunk";
    return false;
  }
  if (audioFormat != 1) {
    err = "not integer PCM (format != 1)";
    return false;
  }
  if (channels != 1) {
    err = "expected mono WAV";
    return false;
  }
  if (bits != 16) {
    err = "expected 16-bit WAV";
    return false;
  }
  if (sampleRate <= 0) {
    err = "invalid sample rate";
    return false;
  }
  if (dataSize < 2 || (dataSize % 2u) != 0) {
    err = "invalid data chunk size";
    return false;
  }

  const auto numFrames = static_cast<size_t>(dataSize / 2u);
  outMonoFloat.resize(numFrames);
  const uint8_t* p = dataPtr;
  for (size_t i = 0; i < numFrames; ++i) {
    const auto s = static_cast<int16_t>(static_cast<uint16_t>(readLe16(p)));
    p += 2;
    outMonoFloat[i] = static_cast<float>(s) / 32768.0f;
  }

  outSampleRate = sampleRate;
  return true;
}
