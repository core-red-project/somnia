#pragma once

#include "ScaleEngine.hpp"
#include "SomniaCompat.hpp"

#include <stdint.h>

class PatternEngine;

enum class PlayMode : uint8_t { Melody = 0, Arpeggio, Chords };

enum class TimeSignature : uint8_t {
    FourFour = 0, // 4/4
    ThreeFour,    // 3/4
    SevenEight    // 7/8
};

struct NoteEvent {
    uint16_t frequency = 0;
    uint16_t duration_ms = 0;
    std_compat::string_view label = "";
    uint8_t midi_note = 60;
    uint8_t velocity = 100;
    bool is_rest = false;
};

struct StreamState {
    size_t step = 0;
    float time = 0.0f;
};

class SequenceEngine {
public:
    SequenceEngine(const ScaleEngine& scaleEngine, const PatternEngine& patternEngine);

    NoteEvent nextEvent(StreamState& state, float timeStep, ScaleType scale, uint32_t seed,
                        uint8_t rootOffset = 0, uint16_t bpm = 0, bool allowRests = false,
                        PlayMode mode = PlayMode::Melody,
                        TimeSignature timeSig = TimeSignature::FourFour) const;

    // Backward compatibility overload
    NoteEvent nextEvent(StreamState& state, float timeStep, bool useLydian, uint32_t seed) const {
        return nextEvent(state, timeStep,
                         useLydian ? ScaleType::Lydian : ScaleType::MinorPentatonic, seed, 0, 0,
                         false, PlayMode::Melody, TimeSignature::FourFour);
    }

private:
    const ScaleEngine& m_scaleEngine;
    const PatternEngine& m_patternEngine;
};
