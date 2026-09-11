#include "MidiEngine.hpp"

#include <fstream>

void MidiEngine::writeBe16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void MidiEngine::writeBe32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void MidiEngine::writeVarLen(std::vector<uint8_t>& buffer, uint32_t value) {
    uint8_t bytes[4];
    int count = 0;
    bytes[count++] = static_cast<uint8_t>(value & 0x7F);
    value >>= 7;
    while (value > 0) {
        bytes[count++] = static_cast<uint8_t>((value & 0x7F) | 0x80);
        value >>= 7;
    }
    for (int i = count - 1; i >= 0; --i) {
        buffer.push_back(bytes[i]);
    }
}

std::vector<uint8_t> MidiEngine::generateMidiData(const std::vector<NoteEvent>& sequence,
                                                  uint16_t bpm) {
    if (bpm == 0) {
        bpm = 120;
    }

    constexpr uint16_t ticksPerQuarter = 480;
    uint32_t tempoUs = 60000000 / bpm;

    std::vector<uint8_t> trackData;

    // Set Tempo Meta Event: FF 51 03 tt tt tt
    writeVarLen(trackData, 0); // delta-time 0
    trackData.push_back(0xFF);
    trackData.push_back(0x51);
    trackData.push_back(0x03);
    trackData.push_back(static_cast<uint8_t>((tempoUs >> 16) & 0xFF));
    trackData.push_back(static_cast<uint8_t>((tempoUs >> 8) & 0xFF));
    trackData.push_back(static_cast<uint8_t>(tempoUs & 0xFF));

    // Time Signature Meta Event: 4/4 (FF 58 04 04 02 18 08)
    writeVarLen(trackData, 0);
    trackData.push_back(0xFF);
    trackData.push_back(0x58);
    trackData.push_back(0x04);
    trackData.push_back(0x04);
    trackData.push_back(0x02);
    trackData.push_back(0x18);
    trackData.push_back(0x08);

    // Track Name Meta Event: "Somnia"
    writeVarLen(trackData, 0);
    trackData.push_back(0xFF);
    trackData.push_back(0x03);
    trackData.push_back(0x06);
    trackData.push_back('S');
    trackData.push_back('o');
    trackData.push_back('m');
    trackData.push_back('n');
    trackData.push_back('i');
    trackData.push_back('a');

    uint32_t deltaAccumulator = 0;

    for (const auto& event : sequence) {
        // Calculate ticks from ms duration: ticks = (duration_ms * 480 * bpm) / 60000
        uint32_t ticks = static_cast<uint32_t>(
            (static_cast<uint64_t>(event.duration_ms) * ticksPerQuarter * bpm) / 60000);
        if (ticks == 0) {
            ticks = 1;
        }

        if (event.is_rest || event.frequency == 0) {
            deltaAccumulator += ticks;
            continue;
        }

        // Note On (Channel 0): 0x90, note, dynamic velocity
        writeVarLen(trackData, deltaAccumulator);
        trackData.push_back(0x90);
        trackData.push_back(event.midi_note & 0x7F);
        trackData.push_back(event.velocity & 0x7F); // Dynamic Velocity

        // Note Off (Channel 0): 0x80, note, velocity 0 after 'ticks' duration
        writeVarLen(trackData, ticks);
        trackData.push_back(0x80);
        trackData.push_back(event.midi_note & 0x7F);
        trackData.push_back(0);

        deltaAccumulator = 0;
    }

    // End of Track Meta Event: FF 2F 00
    writeVarLen(trackData, deltaAccumulator);
    trackData.push_back(0xFF);
    trackData.push_back(0x2F);
    trackData.push_back(0x00);

    // Assemble final SMF Format 0 file
    std::vector<uint8_t> midiFile;

    // Header Chunk "MThd"
    midiFile.push_back('M');
    midiFile.push_back('T');
    midiFile.push_back('h');
    midiFile.push_back('d');
    writeBe32(midiFile, 6);               // Header length = 6 bytes
    writeBe16(midiFile, 0);               // Format 0 (single track)
    writeBe16(midiFile, 1);               // Number of tracks = 1
    writeBe16(midiFile, ticksPerQuarter); // Time division

    // Track Chunk "MTrk"
    midiFile.push_back('M');
    midiFile.push_back('T');
    midiFile.push_back('r');
    midiFile.push_back('k');
    writeBe32(midiFile, static_cast<uint32_t>(trackData.size()));
    midiFile.insert(midiFile.end(), trackData.begin(), trackData.end());

    return midiFile;
}

bool MidiEngine::exportToFile(const std::string& filepath, const std::vector<NoteEvent>& sequence,
                              uint16_t bpm) {
    auto data = generateMidiData(sequence, bpm);
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
    return file.good();
}
