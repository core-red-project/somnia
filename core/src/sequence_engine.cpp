#include "sequence_engine.hpp"

#include "pattern_engine.hpp"
#include "scale_engine.hpp"
#include "somnia_compat.hpp"

SequenceEngine::SequenceEngine(const ScaleEngine& scaleEngine, const PatternEngine& patternEngine)
    : m_scaleEngine(scaleEngine), m_patternEngine(patternEngine) {
}

static size_t applyPlayMode(size_t index, size_t step, PlayMode mode) {
    if (mode == PlayMode::Arpeggio) {
        static constexpr size_t arpeggioOffsets[4] = {0, 2, 4, 7};
        return index + arpeggioOffsets[step % 4];
    }
    if (mode == PlayMode::Chords) {
        size_t chordBase = (step / 4) % 7;
        return chordBase + (index % 3) * 2;
    }
    return index;
}

static uint16_t calculateDuration(size_t rhythmIndex, uint16_t bpm, TimeSignature timeSig) {
    if (bpm > 0) {
        uint16_t safeBpm =
            std_compat::clamp(bpm, static_cast<uint16_t>(20), static_cast<uint16_t>(300));
        uint16_t beatMs = static_cast<uint16_t>(60000 / safeBpm);
        uint16_t sixteenth = beatMs / 4 > 0 ? beatMs / 4 : 1;
        uint16_t eighth = beatMs / 2;
        uint16_t quarter = beatMs;
        uint16_t half = beatMs * 2;

        if (timeSig == TimeSignature::ThreeFour) {
            uint16_t dottedQuarter = beatMs + (beatMs / 2);
            std_compat::array<uint16_t, 3> ternarySubdivisions = {eighth, quarter, dottedQuarter};
            return ternarySubdivisions[rhythmIndex % 3];
        }
        if (timeSig == TimeSignature::SevenEight) {
            uint16_t eighthNote = beatMs / 2;
            std_compat::array<uint16_t, 3> sevenEightSubdivisions = {
                eighthNote,
                static_cast<uint16_t>(eighthNote * 2),
                static_cast<uint16_t>(eighthNote * 3)};
            return sevenEightSubdivisions[rhythmIndex % 3];
        }
        std_compat::array<uint16_t, 4> bpmSubdivisions = {sixteenth, eighth, quarter, half};
        return bpmSubdivisions[rhythmIndex % 4];
    }

    static constexpr std_compat::array<uint16_t, 3> defaultSubdivisions = {100, 200, 400};
    return defaultSubdivisions[rhythmIndex % 3];
}

NoteEvent SequenceEngine::nextEvent(StreamState& state,
                                    float timeStep,
                                    ScaleType scale,
                                    uint32_t seed,
                                    uint8_t rootOffset,
                                    uint16_t bpm,
                                    bool allowRests,
                                    PlayMode mode,
                                    TimeSignature timeSig) const {
    // Sample pitch index using multi-octave FBM with non-degrading integer cell + fraction
    float pitchSample = m_patternEngine.getFbmSample(state.cell, state.frac, seed, 2);
    float scaledPitch = std_compat::clamp(pitchSample * 100.0f, 0.0f, 100.0f);
    size_t index = applyPlayMode(static_cast<size_t>(scaledPitch), state.step, mode);

    Note note = m_scaleEngine.getNote(index, scale, rootOffset);

    // Calculate dynamic velocity (40 to 127) aligned with Q8 formula
    float velocitySample = m_patternEngine.getSample(state.cell + 25, state.frac, seed);
    uint8_t velocity =
        static_cast<uint8_t>(std_compat::clamp(40.0f + velocitySample * 87.0f, 40.0f, 127.0f));

    // Sample rhythm using coordinate offset
    float rhythmSample = m_patternEngine.getFbmSample(state.cell + 50, state.frac, seed, 2);
    size_t numChoices = (bpm > 0 && timeSig == TimeSignature::FourFour) ? 4 : 3;
    float scaledRhythm = std_compat::clamp(
        rhythmSample * static_cast<float>(numChoices), 0.0f, static_cast<float>(numChoices - 1));
    size_t rhythmIndex = static_cast<size_t>(scaledRhythm);
    uint16_t duration = calculateDuration(rhythmIndex, bpm, timeSig);

    bool isRest = false;
    if (allowRests) {
        float restSample = m_patternEngine.getSample(state.cell + 100, state.frac, seed);
        if (restSample < 0.18f) {
            isRest = true;
        }
    }

    NoteEvent event(isRest ? static_cast<uint16_t>(0) : note.frequency,
                    duration,
                    isRest ? std_compat::string_view("REST") : note.name,
                    note.midi_note,
                    velocity,
                    isRest);

    // Monotonic phase progression without precision loss
    state.frac += timeStep;
    if (state.frac >= 1.0f) {
        uint32_t whole = static_cast<uint32_t>(state.frac);
        state.cell += whole;
        state.frac -= static_cast<float>(whole);
    }
    state.time = static_cast<float>(state.cell) + state.frac;
    state.step++;

    return event;
}

NoteEvent SequenceEngine::nextEventQ8(StreamStateQ8& state,
                                      int32_t timeStepQ8,
                                      ScaleType scale,
                                      uint32_t seed,
                                      uint8_t rootOffset,
                                      uint16_t bpm,
                                      bool allowRests,
                                      PlayMode mode,
                                      TimeSignature timeSig) const {
    uint32_t t = state.time_q8;

    // Sample pitch index using multi-octave FBM in pure integer Q8.8
    uint8_t pitchSampleQ8 = m_patternEngine.getFbmSampleQ8(t, seed, 2);
    size_t index = (static_cast<size_t>(pitchSampleQ8) * 100) / 255;
    if (index > 100) {
        index = 100;
    }
    index = applyPlayMode(index, state.step, mode);

    Note note = m_scaleEngine.getNote(index, scale, rootOffset);

    // Calculate dynamic velocity (40 to 127) using integer offset t + (25 << 8)
    uint8_t velocitySampleQ8 = m_patternEngine.getSampleQ8(t + (25 << 8), seed);
    uint8_t velocity =
        static_cast<uint8_t>(40 + ((static_cast<uint16_t>(velocitySampleQ8) * 87) / 255));

    // Sample rhythm using coordinate offset t + (50 << 8)
    uint8_t rhythmSampleQ8 = m_patternEngine.getFbmSampleQ8(t + (50 << 8), seed, 2);
    size_t numChoices = (bpm > 0 && timeSig == TimeSignature::FourFour) ? 4 : 3;
    size_t rhythmIndex = (static_cast<size_t>(rhythmSampleQ8) * numChoices) / 256;
    if (rhythmIndex >= numChoices) {
        rhythmIndex = numChoices - 1;
    }
    uint16_t duration = calculateDuration(rhythmIndex, bpm, timeSig);

    bool isRest = false;
    if (allowRests) {
        uint8_t restSampleQ8 = m_patternEngine.getSampleQ8(t + (100 << 8), seed);
        if (restSampleQ8 < 46) {
            isRest = true;
        }
    }

    NoteEvent event(isRest ? static_cast<uint16_t>(0) : note.frequency,
                    duration,
                    isRest ? std_compat::string_view("REST") : note.name,
                    note.midi_note,
                    velocity,
                    isRest);

    state.time_q8 += static_cast<uint32_t>(timeStepQ8);
    state.step++;

    return event;
}
