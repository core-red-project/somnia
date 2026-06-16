#pragma once

#include <array>
#include <cstdint>
#include <string_view>

struct Note {
    std::string_view name;
    uint16_t frequency;
};

class ScaleEngine {
public:
    ScaleEngine() = default;

    Note getNote(size_t index, bool useLydian) const;

private:
    static constexpr std::array<int, 5> m_minorPentatonicIntervals = {0, 3, 5, 7, 10};
    static constexpr std::array<int, 7> m_lydianIntervals = {0, 2, 4, 6, 7, 9, 11};
    static constexpr std::array<std::string_view, 12> m_noteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};
