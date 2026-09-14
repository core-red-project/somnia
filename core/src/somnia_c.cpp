#include "somnia_c.h"

#include "MidiEngine.hpp"
#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SequenceEngine.hpp"
#include "WavEngine.hpp"

#include <vector>

struct SomniaEngineContext {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine;
    StreamState state;

    SomniaEngineContext()
        : scaleEngine(), patternEngine(), sequenceEngine(scaleEngine, patternEngine), state() {
    }
};

extern "C" {

somnia_engine_t somnia_create(void) {
    return new SomniaEngineContext();
}

void somnia_destroy(somnia_engine_t engine) {
    if (engine) {
        delete static_cast<SomniaEngineContext*>(engine);
    }
}

somnia_c_note_event_t somnia_next_event(somnia_engine_t engine, uint32_t seed, uint8_t scale_type,
                                        uint8_t root_offset, uint16_t bpm, bool allow_rests,
                                        uint8_t play_mode, uint8_t time_sig) {
    somnia_c_note_event_t out{};
    if (!engine) {
        return out;
    }

    auto* ctx = static_cast<SomniaEngineContext*>(engine);
    ScaleType scale = static_cast<ScaleType>(scale_type % 10);
    PlayMode mode = static_cast<PlayMode>(play_mode % 3);
    TimeSignature sig = static_cast<TimeSignature>(time_sig % 3);

    NoteEvent ev = ctx->sequenceEngine.nextEvent(ctx->state, 0.25f, scale, seed, root_offset, bpm,
                                                 allow_rests, mode, sig);

    out.frequency = ev.frequency;
    out.duration_ms = ev.duration_ms;
    out.label = ev.label.data();
    out.midi_note = ev.midi_note;
    out.velocity = ev.velocity;
    out.is_rest = ev.is_rest;

    return out;
}

bool somnia_export_midi(const char* filepath, uint32_t seed, uint8_t scale_type,
                        uint8_t root_offset, uint16_t bpm, size_t steps) {
    if (!filepath || steps == 0)
        return false;

    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);
    StreamState state{};

    ScaleType scale = static_cast<ScaleType>(scale_type % 10);
    std::vector<NoteEvent> sequence;
    sequence.reserve(steps);

    for (size_t i = 0; i < steps; ++i) {
        sequence.push_back(
            sequenceEngine.nextEvent(state, 0.25f, scale, seed, root_offset, bpm, true));
    }

    return MidiEngine::exportToFile(filepath, sequence, bpm);
}

bool somnia_export_wav(const char* filepath, uint32_t seed, uint8_t scale_type, uint8_t root_offset,
                       uint16_t bpm, size_t steps) {
    if (!filepath || steps == 0)
        return false;

    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);
    StreamState state{};

    ScaleType scale = static_cast<ScaleType>(scale_type % 10);
    std::vector<NoteEvent> sequence;
    sequence.reserve(steps);

    for (size_t i = 0; i < steps; ++i) {
        sequence.push_back(
            sequenceEngine.nextEvent(state, 0.25f, scale, seed, root_offset, bpm, true));
    }

    return WavEngine::exportToFile(filepath, sequence, 44100);
}

const char* somnia_version(void) {
    return "0.5.0";
}

} // extern "C"
