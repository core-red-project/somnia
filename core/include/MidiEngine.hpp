#pragma once

#include "SequenceEngine.hpp"

#include <cstdint>
#include <string>
#include <vector>

class MidiEngine {
public:
    // Exports sequence to a Standard MIDI File (SMF Format 0) byte vector
    static std::vector<uint8_t> generateMidiData(const std::vector<NoteEvent>& sequence,
                                                 uint16_t bpm = 120);

    // Writes the MIDI file directly to disk
    static bool exportToFile(const std::string& filepath, const std::vector<NoteEvent>& sequence,
                             uint16_t bpm = 120);

private:
    static void writeVarLen(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeBe32(std::vector<uint8_t>& buffer, uint32_t value);
    static void writeBe16(std::vector<uint8_t>& buffer, uint16_t value);
};
