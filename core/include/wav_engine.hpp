#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "sequence_engine.hpp"
#include "somnia_compat.hpp"

class WavEngine {
public:
    // Generates complete 16-bit 44.1kHz mono WAV file data
    static std::vector<uint8_t> generateWavData(std_compat::span<const NoteEvent> sequence,
                                                uint32_t sampleRate = 44100);

    // Generates raw 16-bit signed PCM samples (useful for piping to audio servers)
    static std::vector<int16_t> synthesizePcm(std_compat::span<const NoteEvent> sequence,
                                              uint32_t sampleRate = 44100);

    // Writes WAV file to disk
    static bool exportToFile(const std::string& filepath,
                             std_compat::span<const NoteEvent> sequence,
                             uint32_t sampleRate = 44100);

    // Backward compatibility overloads for std::vector
    static std::vector<uint8_t> generateWavData(const std::vector<NoteEvent>& sequence,
                                                uint32_t sampleRate = 44100) {
        return generateWavData(std_compat::span<const NoteEvent>(sequence.data(), sequence.size()),
                               sampleRate);
    }
    static std::vector<int16_t> synthesizePcm(const std::vector<NoteEvent>& sequence,
                                              uint32_t sampleRate = 44100) {
        return synthesizePcm(std_compat::span<const NoteEvent>(sequence.data(), sequence.size()),
                             sampleRate);
    }
    static bool exportToFile(const std::string& filepath,
                             const std::vector<NoteEvent>& sequence,
                             uint32_t sampleRate = 44100) {
        return exportToFile(filepath,
                            std_compat::span<const NoteEvent>(sequence.data(), sequence.size()),
                            sampleRate);
    }

private:
    static void writeLe32(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeLe16(std::vector<uint8_t>& buffer, uint16_t value);
};
