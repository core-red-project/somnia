#include "SequenceEngine.hpp"

#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SomniaCompat.hpp"

SequenceEngine::SequenceEngine(const ScaleEngine& scaleEngine, const PatternEngine& patternEngine)
    : m_scaleEngine(scaleEngine), m_patternEngine(patternEngine) {
}

NoteEvent SequenceEngine::nextEvent(StreamState& state, float timeStep, ScaleType scale,
                                    uint32_t seed, uint8_t rootOffset, uint16_t bpm,
                                    bool allowRests, PlayMode mode, TimeSignature timeSig) const {
    float t = state.time;

    // Sample pitch index using multi-octave FBM
    float pitchSample = m_patternEngine.getFbmSample(t, seed, 2);
    float scaledPitch = std_compat::clamp(pitchSample * 100.0f, 0.0f, 100.0f);
    size_t index = static_cast<size_t>(scaledPitch);

    if (mode == PlayMode::Arpeggio) {
        // Diatonic arpeggiator offsets: Root, 3rd, 5th, Octave
        static constexpr size_t arpeggioOffsets[4] = {0, 2, 4, 7};
        index = index + arpeggioOffsets[state.step % 4];
    } else if (mode == PlayMode::Chords) {
        // Harmonic root movement: shift every 4 steps
        size_t chordBase = (state.step / 4) % 7;
        index = chordBase + (index % 3) * 2;
    }

    Note note = m_scaleEngine.getNote(index, scale, rootOffset);

    // Calculate dynamic velocity (40 to 127) from noise gradient
    float velocitySample = m_patternEngine.getSample(t + 25.0f, seed);
    uint8_t velocity =
        static_cast<uint8_t>(std_compat::clamp(velocitySample * 65.0f + 62.0f, 40.0f, 127.0f));

    // Sample rhythm using coordinate offset
    float rhythmSample = m_patternEngine.getFbmSample(t + 50.0f, seed, 2);

    uint16_t duration = 200;
    if (bpm > 0) {
        uint16_t beatMs = static_cast<uint16_t>(60000 / bpm);
        uint16_t sixteenth = beatMs / 4 > 0 ? beatMs / 4 : 1;
        uint16_t eighth = beatMs / 2;
        uint16_t quarter = beatMs;
        uint16_t half = beatMs * 2;

        if (timeSig == TimeSignature::ThreeFour) {
            // 3/4 Waltz / Ternary subdivisions: eighth, quarter, dotted-quarter (beatMs * 1.5)
            uint16_t dottedQuarter = beatMs + (beatMs / 2);
            std_compat::array<uint16_t, 3> ternarySubdivisions = {eighth, quarter, dottedQuarter};
            float scaled = std_compat::clamp(rhythmSample * 3.0f, 0.0f, 2.0f);
            duration = ternarySubdivisions[static_cast<size_t>(scaled) % 3];
        } else if (timeSig == TimeSignature::SevenEight) {
            // 7/8 Complex meter: 2+2+3 additive groupings of eighth notes
            uint16_t eighthNote = beatMs / 2;
            std_compat::array<uint16_t, 3> sevenEightSubdivisions = {
                eighthNote, static_cast<uint16_t>(eighthNote * 2),
                static_cast<uint16_t>(eighthNote * 3)};
            float scaled = std_compat::clamp(rhythmSample * 3.0f, 0.0f, 2.0f);
            duration = sevenEightSubdivisions[static_cast<size_t>(scaled) % 3];
        } else {
            // Standard 4/4
            std_compat::array<uint16_t, 4> bpmSubdivisions = {sixteenth, eighth, quarter, half};
            float scaledRhythm = std_compat::clamp(rhythmSample * 4.0f, 0.0f, 3.0f);
            size_t rhythmIndex = static_cast<size_t>(scaledRhythm);
            duration = bpmSubdivisions[rhythmIndex % 4];
        }
    } else {
        static constexpr std_compat::array<uint16_t, 3> defaultSubdivisions = {100, 200, 400};
        float scaledRhythm = std_compat::clamp(rhythmSample * 3.0f, 0.0f, 2.0f);
        size_t rhythmIndex = static_cast<size_t>(scaledRhythm);
        duration = defaultSubdivisions[rhythmIndex % 3];
    }

    bool isRest = false;
    if (allowRests) {
        float restSample = m_patternEngine.getSample(t + 100.0f, seed);
        if (restSample < 0.18f) {
            isRest = true;
        }
    }

    NoteEvent event{.frequency = isRest ? static_cast<uint16_t>(0) : note.frequency,
                    .duration_ms = duration,
                    .label = isRest ? std_compat::string_view("REST") : note.name,
                    .midi_note = note.midi_note,
                    .velocity = velocity,
                    .is_rest = isRest};

    state.time += timeStep;
    state.step++;

    return event;
}
