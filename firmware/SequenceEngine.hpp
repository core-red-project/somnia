#pragma once

#include "SomniaCompat.hpp"

#include <stdint.h>

class ScaleEngine;
class PatternEngine;

struct NoteEvent {
    uint16_t frequency;
    uint16_t duration_ms;
    std_compat::string_view label;
};

struct StreamState {
    size_t step = 0;
    float time = 0.0f;
};

class SequenceEngine {
public:
    SequenceEngine(const ScaleEngine& scaleEngine, const PatternEngine& patternEngine);

    NoteEvent nextEvent(StreamState& state, float timeStep, bool useLydian, uint32_t seed) const;

private:
    const ScaleEngine& m_scaleEngine;
    const PatternEngine& m_patternEngine;
};
