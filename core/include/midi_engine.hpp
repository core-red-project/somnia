#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "sequence_engine.hpp"
#include "somnia_compat.hpp"

class MidiEngine {
public:
    // Exports sequence to a Standard MIDI File (SMF Format 0) byte vector
    static std::vector<uint8_t> generateMidiData(std_compat::span<const NoteEvent> sequence,
                                                 uint16_t bpm = 120,
                                                 TimeSignature timeSig = TimeSignature::FourFour);

    // Writes the MIDI file directly to disk
    static bool exportToFile(const std::string& filepath,
                             std_compat::span<const NoteEvent> sequence,
                             uint16_t bpm = 120,
                             TimeSignature timeSig = TimeSignature::FourFour);

    // Backward compatibility overloads for std::vector
    static std::vector<uint8_t> generateMidiData(const std::vector<NoteEvent>& sequence,
                                                 uint16_t bpm = 120,
                                                 TimeSignature timeSig = TimeSignature::FourFour) {
        return generateMidiData(
            std_compat::span<const NoteEvent>(sequence.data(), sequence.size()), bpm, timeSig);
    }
    static bool exportToFile(const std::string& filepath,
                             const std::vector<NoteEvent>& sequence,
                             uint16_t bpm = 120,
                             TimeSignature timeSig = TimeSignature::FourFour) {
        return exportToFile(filepath,
                            std_compat::span<const NoteEvent>(sequence.data(), sequence.size()),
                            bpm,
                            timeSig);
    }

private:
    static void writeVarLen(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeBe32(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeBe16(std::vector<uint8_t>& buffer, uint16_t value);
};
