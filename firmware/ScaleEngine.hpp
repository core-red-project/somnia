#pragma once

#include "SomniaCompat.hpp"

#include <stdint.h>

struct Note {
    std_compat::string_view name;
    uint16_t frequency = 0;
    uint8_t midi_note = 60;
    bool is_rest = false;

    constexpr Note() = default;
    constexpr Note(std_compat::string_view n, uint16_t f, uint8_t m, bool r = false)
        : name(n), frequency(f), midi_note(m), is_rest(r) {
    }
};

enum class ScaleType : uint8_t {
    Lydian = 0,
    MinorPentatonic,
    MajorPentatonic,
    Dorian,
    NaturalMinor,
    Major,
    Mixolydian,
    Phrygian,
    Blues,
    Hirajoshi
};

class ScaleEngine {
public:
    ScaleEngine() = default;

    Note getNote(size_t index, ScaleType scale, uint8_t rootOffset = 0, int baseOctave = 4) const;

    // Backward compatibility overload
    Note getNote(size_t index, bool useLydian) const {
        return getNote(index, useLydian ? ScaleType::Lydian : ScaleType::MinorPentatonic, 0, 4);
    }

    static ScaleType parseScale(std_compat::string_view name);
    static std_compat::string_view scaleName(ScaleType scale);
    static uint8_t parseRootNote(std_compat::string_view name);
    static std_compat::string_view rootNoteName(uint8_t rootOffset);

    // 12-TET precalculated frequencies LUT (Look-Up Table) for MIDI notes 0..127
    static uint16_t getFrequencyForMidi(uint8_t midiNote);

private:
    static constexpr std_compat::array<int, 7> m_lydianIntervals = {0, 2, 4, 6, 7, 9, 11};
    static constexpr std_compat::array<int, 5> m_minorPentatonicIntervals = {0, 3, 5, 7, 10};
    static constexpr std_compat::array<int, 5> m_majorPentatonicIntervals = {0, 2, 4, 7, 9};
    static constexpr std_compat::array<int, 7> m_dorianIntervals = {0, 2, 3, 5, 7, 9, 10};
    static constexpr std_compat::array<int, 7> m_naturalMinorIntervals = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std_compat::array<int, 7> m_majorIntervals = {0, 2, 4, 5, 7, 9, 11};
    static constexpr std_compat::array<int, 7> m_mixolydianIntervals = {0, 2, 4, 5, 7, 9, 10};
    static constexpr std_compat::array<int, 7> m_phrygianIntervals = {0, 1, 3, 5, 7, 8, 10};
    static constexpr std_compat::array<int, 6> m_bluesIntervals = {0, 3, 5, 6, 7, 10};
    static constexpr std_compat::array<int, 5> m_hirajoshiIntervals = {0, 2, 3, 7, 8};

    static constexpr std_compat::array<std_compat::string_view, 12> m_noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};
