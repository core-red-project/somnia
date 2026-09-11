#include "ScaleEngine.hpp"

#include "SomniaCompat.hpp"

#include <string.h>

#if __cplusplus < 201703L
constexpr std_compat::array<int, 7> ScaleEngine::m_lydianIntervals;
constexpr std_compat::array<int, 5> ScaleEngine::m_minorPentatonicIntervals;
constexpr std_compat::array<int, 5> ScaleEngine::m_majorPentatonicIntervals;
constexpr std_compat::array<int, 7> ScaleEngine::m_dorianIntervals;
constexpr std_compat::array<int, 7> ScaleEngine::m_naturalMinorIntervals;
constexpr std_compat::array<int, 7> ScaleEngine::m_majorIntervals;
constexpr std_compat::array<int, 7> ScaleEngine::m_mixolydianIntervals;
constexpr std_compat::array<int, 7> ScaleEngine::m_phrygianIntervals;
constexpr std_compat::array<int, 6> ScaleEngine::m_bluesIntervals;
constexpr std_compat::array<int, 5> ScaleEngine::m_hirajoshiIntervals;
constexpr std_compat::array<std_compat::string_view, 12> ScaleEngine::m_noteNames;
#endif

// Precalculated 12-TET frequencies (Hz) for MIDI notes 0 to 127
// A4 (MIDI 69) = 440Hz, C4 (MIDI 60) = 262Hz
static constexpr uint16_t kMidiFrequencies[128] SOMNIA_PROGMEM = {
    8,    9,    9,    10,   10,    11,    12,    12,   13,   14,   15,   15,  // 0-11   (C-1 to B-1)
    16,   17,   18,   19,   21,    22,    23,    24,   26,   28,   29,   31,  // 12-23  (C0 to B0)
    33,   35,   37,   39,   41,    44,    46,    49,   52,   55,   58,   62,  // 24-35  (C1 to B1)
    65,   69,   73,   78,   82,    87,    92,    98,   104,  110,  117,  123, // 36-47  (C2 to B2)
    131,  139,  147,  156,  165,   175,   185,   196,  208,  220,  233,  247, // 48-59  (C3 to B3)
    262,  277,  294,  311,  330,   349,   370,   392,  415,  440,  466,  494, // 60-71  (C4 to B4)
    523,  554,  587,  622,  659,   698,   740,   784,  831,  880,  932,  988, // 72-83  (C5 to B5)
    1047, 1109, 1175, 1245, 1319,  1397,  1480,  1568, 1661, 1760, 1865, 1976, // 84-95  (C6 to B6)
    2093, 2217, 2349, 2489, 2637,  2794,  2960,  3136, 3322, 3520, 3729, 3951, // 96-107 (C7 to B7)
    4186, 4435, 4699, 4978, 5274,  5588,  5920,  6272, 6645, 7040, 7459, 7902, // 108-119(C8 to B8)
    8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544                         // 120-127(C9 to G9)
};

static constexpr std_compat::string_view kMidiNoteLabels[128] = {
    "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1", "C0",
    "C#0", "D0",   "D#0", "E0",   "F0",  "F#0", "G0",   "G#0", "A0",   "A#0", "B0",   "C1",  "C#1",
    "D1",  "D#1",  "E1",  "F1",   "F#1", "G1",  "G#1",  "A1",  "A#1",  "B1",  "C2",   "C#2", "D2",
    "D#2", "E2",   "F2",  "F#2",  "G2",  "G#2", "A2",   "A#2", "B2",   "C3",  "C#3",  "D3",  "D#3",
    "E3",  "F3",   "F#3", "G3",   "G#3", "A3",  "A#3",  "B3",  "C4",   "C#4", "D4",   "D#4", "E4",
    "F4",  "F#4",  "G4",  "G#4",  "A4",  "A#4", "B4",   "C5",  "C#5",  "D5",  "D#5",  "E5",  "F5",
    "F#5", "G5",   "G#5", "A5",   "A#5", "B5",  "C6",   "C#6", "D6",   "D#6", "E6",   "F6",  "F#6",
    "G6",  "G#6",  "A6",  "A#6",  "B6",  "C7",  "C#7",  "D7",  "D#7",  "E7",  "F7",   "F#7", "G7",
    "G#7", "A7",   "A#7", "B7",   "C8",  "C#8", "D8",   "D#8", "E8",   "F8",  "F#8",  "G8",  "G#8",
    "A8",  "A#8",  "B8",  "C9",   "C#9", "D9",  "D#9",  "E9",  "F9",   "F#9", "G9"};

uint16_t ScaleEngine::getFrequencyForMidi(uint8_t midiNote) {
    if (midiNote >= 128) {
        midiNote = 127;
    }
    return SOMNIA_READ_WORD(&kMidiFrequencies[midiNote]);
}

Note ScaleEngine::getNote(size_t index, ScaleType scale, uint8_t rootOffset, int baseOctave) const {
    std_compat::span<const int> intervals;

    switch (scale) {
        case ScaleType::Lydian:
            intervals = std_compat::span<const int>(m_lydianIntervals);
            break;
        case ScaleType::MinorPentatonic:
            intervals = std_compat::span<const int>(m_minorPentatonicIntervals);
            break;
        case ScaleType::MajorPentatonic:
            intervals = std_compat::span<const int>(m_majorPentatonicIntervals);
            break;
        case ScaleType::Dorian:
            intervals = std_compat::span<const int>(m_dorianIntervals);
            break;
        case ScaleType::NaturalMinor:
            intervals = std_compat::span<const int>(m_naturalMinorIntervals);
            break;
        case ScaleType::Major:
            intervals = std_compat::span<const int>(m_majorIntervals);
            break;
        case ScaleType::Mixolydian:
            intervals = std_compat::span<const int>(m_mixolydianIntervals);
            break;
        case ScaleType::Phrygian:
            intervals = std_compat::span<const int>(m_phrygianIntervals);
            break;
        case ScaleType::Blues:
            intervals = std_compat::span<const int>(m_bluesIntervals);
            break;
        case ScaleType::Hirajoshi:
            intervals = std_compat::span<const int>(m_hirajoshiIntervals);
            break;
        default:
            intervals = std_compat::span<const int>(m_lydianIntervals);
            break;
    }

    size_t scaleSize = intervals.size();
    size_t step = index % scaleSize;
    int octaveShift = static_cast<int>((index / scaleSize) % 4);

    // Root note calculation
    rootOffset = rootOffset % 12;
    int baseMidi = (baseOctave + 1) * 12 + rootOffset;
    int noteMidi = baseMidi + intervals[step] + (octaveShift * 12);

    if (noteMidi < 0) {
        noteMidi = 0;
    } else if (noteMidi > 127) {
        noteMidi = 127;
    }

    uint8_t midiNumber = static_cast<uint8_t>(noteMidi);
    uint16_t frequency = getFrequencyForMidi(midiNumber);
    std_compat::string_view label = kMidiNoteLabels[midiNumber];

    return Note(label, frequency, midiNumber, false);
}

ScaleType ScaleEngine::parseScale(std_compat::string_view name) {
    const char* str = name.data();
    if (!str)
        return ScaleType::Lydian;

    if (strcmp(str, "lydian") == 0)
        return ScaleType::Lydian;
    if (strcmp(str, "minor-pentatonic") == 0 || strcmp(str, "pentatonic") == 0)
        return ScaleType::MinorPentatonic;
    if (strcmp(str, "major-pentatonic") == 0)
        return ScaleType::MajorPentatonic;
    if (strcmp(str, "dorian") == 0)
        return ScaleType::Dorian;
    if (strcmp(str, "minor") == 0 || strcmp(str, "aeolian") == 0 ||
        strcmp(str, "natural-minor") == 0)
        return ScaleType::NaturalMinor;
    if (strcmp(str, "major") == 0 || strcmp(str, "ionian") == 0)
        return ScaleType::Major;
    if (strcmp(str, "mixolydian") == 0)
        return ScaleType::Mixolydian;
    if (strcmp(str, "phrygian") == 0)
        return ScaleType::Phrygian;
    if (strcmp(str, "blues") == 0)
        return ScaleType::Blues;
    if (strcmp(str, "hirajoshi") == 0)
        return ScaleType::Hirajoshi;

    return ScaleType::Lydian;
}

std_compat::string_view ScaleEngine::scaleName(ScaleType scale) {
    switch (scale) {
        case ScaleType::Lydian:
            return "lydian";
        case ScaleType::MinorPentatonic:
            return "minor-pentatonic";
        case ScaleType::MajorPentatonic:
            return "major-pentatonic";
        case ScaleType::Dorian:
            return "dorian";
        case ScaleType::NaturalMinor:
            return "natural-minor";
        case ScaleType::Major:
            return "major";
        case ScaleType::Mixolydian:
            return "mixolydian";
        case ScaleType::Phrygian:
            return "phrygian";
        case ScaleType::Blues:
            return "blues";
        case ScaleType::Hirajoshi:
            return "hirajoshi";
        default:
            return "lydian";
    }
}

uint8_t ScaleEngine::parseRootNote(std_compat::string_view name) {
    const char* str = name.data();
    if (!str)
        return 0; // Default C

    if (strcmp(str, "C") == 0 || strcmp(str, "c") == 0)
        return 0;
    if (strcmp(str, "C#") == 0 || strcmp(str, "c#") == 0 || strcmp(str, "Db") == 0 ||
        strcmp(str, "db") == 0)
        return 1;
    if (strcmp(str, "D") == 0 || strcmp(str, "d") == 0)
        return 2;
    if (strcmp(str, "D#") == 0 || strcmp(str, "d#") == 0 || strcmp(str, "Eb") == 0 ||
        strcmp(str, "eb") == 0)
        return 3;
    if (strcmp(str, "E") == 0 || strcmp(str, "e") == 0)
        return 4;
    if (strcmp(str, "F") == 0 || strcmp(str, "f") == 0)
        return 5;
    if (strcmp(str, "F#") == 0 || strcmp(str, "f#") == 0 || strcmp(str, "Gb") == 0 ||
        strcmp(str, "gb") == 0)
        return 6;
    if (strcmp(str, "G") == 0 || strcmp(str, "g") == 0)
        return 7;
    if (strcmp(str, "G#") == 0 || strcmp(str, "g#") == 0 || strcmp(str, "Ab") == 0 ||
        strcmp(str, "ab") == 0)
        return 8;
    if (strcmp(str, "A") == 0 || strcmp(str, "a") == 0)
        return 9;
    if (strcmp(str, "A#") == 0 || strcmp(str, "a#") == 0 || strcmp(str, "Bb") == 0 ||
        strcmp(str, "bb") == 0)
        return 10;
    if (strcmp(str, "B") == 0 || strcmp(str, "b") == 0)
        return 11;

    return 0;
}

std_compat::string_view ScaleEngine::rootNoteName(uint8_t rootOffset) {
    return m_noteNames[rootOffset % 12];
}
