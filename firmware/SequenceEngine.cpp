#include "SequenceEngine.hpp"

#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SomniaCompat.hpp"

SequenceEngine::SequenceEngine(const ScaleEngine& scaleEngine, const PatternEngine& patternEngine)
    : m_scaleEngine(scaleEngine), m_patternEngine(patternEngine) {
}

NoteEvent SequenceEngine::nextEvent(StreamState& state, float timeStep, bool useLydian,
                                    uint32_t seed) const {
    // Quantized physical subdivisions
    static constexpr std_compat::array<uint16_t, 3> subdivisions = {100, 200, 400};

    float t = state.time;

    // Sample pitch index using multi-octave FBM
    float pitchSample = m_patternEngine.getFbmSample(t, seed, 2);
    float scaledPitch = std_compat::clamp(pitchSample * 100.0f, 0.0f, 100.0f);
    size_t index = static_cast<size_t>(scaledPitch);

    Note note = m_scaleEngine.getNote(index, useLydian);

    // Sample rhythm using coordinate offset
    float rhythmSample = m_patternEngine.getFbmSample(t + 50.0f, seed, 2);
    float scaledRhythm = std_compat::clamp(rhythmSample * static_cast<float>(subdivisions.size()),
                                           0.0f, static_cast<float>(subdivisions.size() - 1));
    size_t rhythmIndex = static_cast<size_t>(scaledRhythm);

    uint16_t duration = subdivisions[rhythmIndex % subdivisions.size()];

    // Construct return event using C++20 designated initializers
    NoteEvent event{.frequency = note.frequency, .duration_ms = duration, .label = note.name};

    // Update stream state statefully (O(1) memory model)
    state.time += timeStep;
    state.step++;

    return event;
}
