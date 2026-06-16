#include "ScaleEngine.hpp"

#include <cmath>
#include <span>

Note ScaleEngine::getNote(size_t index, bool useLydian) const {
    std::span<const int> intervals;
    if (useLydian) {
        intervals = m_lydianIntervals;
    } else {
        intervals = m_minorPentatonicIntervals;
    }

    size_t scaleSize = intervals.size();
    size_t step = index % scaleSize;

    // Limit octave shift to a safe range [0, 3] to keep octaves within [4, 8]
    int octaveShift = static_cast<int>((index / scaleSize) % 4);
    int semitones = intervals[step] + octaveShift * 12;

    // Convert semitones from root A4 to be relative to C4 (A4 is 9 semitones above C4)
    int semitonesFromC4 = semitones + 9;
    int noteIdx = (semitonesFromC4 % 12 + 12) % 12;
    int octave = 4 + static_cast<int>(std::floor(static_cast<float>(semitonesFromC4) / 12.0f));

    // Clamp octave index to valid table row [0, 4] corresponding to octaves [4, 8]
    int octaveIndex = octave - 4;
    if (octaveIndex < 0) {
        octaveIndex = 0;
    } else if (octaveIndex > 4) {
        octaveIndex = 4;
    }

    static constexpr std::string_view notesTable[5][12] = {
        {"C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4"},
        {"C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5"},
        {"C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6"},
        {"C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7"},
        {"C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8"}};

    std::string_view name = notesTable[octaveIndex][noteIdx];

    // Calculate frequency via 12-TET Equal Temperament formula (Root A4 = 440.0Hz)
    float freq = 440.0f * std::pow(2.0f, static_cast<float>(semitones) / 12.0f);
    uint16_t frequency = static_cast<uint16_t>(std::round(freq));

    return Note{name, frequency};
}
