#pragma once

#include "SequenceEngine.hpp"

#include <cstdint>
#include <string>
#include <vector>

class WavEngine {
public:
    // Generates complete 16-bit 44.1kHz mono WAV file data
    static std::vector<uint8_t> generateWavData(const std::vector<NoteEvent>& sequence,
                                                uint32_t sampleRate = 44100);

    // Generates raw 16-bit signed PCM samples (useful for piping to audio servers)
    static std::vector<int16_t> synthesizePcm(const std::vector<NoteEvent>& sequence,
                                              uint32_t sampleRate = 44100);

    // Writes WAV file to disk
    static bool exportToFile(const std::string& filepath, const std::vector<NoteEvent>& sequence,
                             uint32_t sampleRate = 44100);

private:
    static void writeLe32(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeLe16(std::vector<uint8_t>& buffer, uint16_t value);
};
