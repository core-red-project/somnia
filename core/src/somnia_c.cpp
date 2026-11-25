#include "somnia_c.h"

#include <new>
#include <vector>

#include "midi_engine.hpp"
#include "pattern_engine.hpp"
#include "scale_engine.hpp"
#include "sequence_engine.hpp"
#include "wav_engine.hpp"

struct SomniaEngineContext {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine;
    StreamState state;

    SomniaEngineContext()
        : scaleEngine(), patternEngine(), sequenceEngine(scaleEngine, patternEngine), state() {
    }
};

static constexpr size_t MAX_EXPORT_STEPS = 1048576; // 1M steps cap against length_error

static inline uint8_t safeNormalizeRoot(int8_t root_offset) {
    int32_t r = static_cast<int32_t>(root_offset);
    return static_cast<uint8_t>(((r % 12) + 12) % 12);
}

extern "C" {

somnia_engine_t somnia_create(void) {
    try {
        return new (std::nothrow) SomniaEngineContext();
    } catch (...) {
        return nullptr;
    }
}

void somnia_destroy(somnia_engine_t engine) {
    try {
        if (engine) {
            delete static_cast<SomniaEngineContext*>(engine);
        }
    } catch (...) {
    }
}

somnia_c_note_event_t somnia_next_event(somnia_engine_t engine,
                                        uint32_t seed,
                                        uint8_t scale_type,
                                        int8_t root_offset,
                                        uint16_t bpm,
                                        bool allow_rests,
                                        uint8_t play_mode,
                                        uint8_t time_sig) {
    somnia_c_note_event_t out{};
    if (!engine) {
        return out;
    }

    try {
        auto* ctx = static_cast<SomniaEngineContext*>(engine);
        ScaleType scale = static_cast<ScaleType>(scale_type % 10);
        PlayMode mode = static_cast<PlayMode>(play_mode % 3);
        TimeSignature sig = static_cast<TimeSignature>(time_sig % 3);
        uint8_t norm_root = safeNormalizeRoot(root_offset);

        NoteEvent ev = ctx->sequenceEngine.nextEvent(
            ctx->state, 0.25f, scale, seed, norm_root, bpm, allow_rests, mode, sig);

        out.frequency = ev.frequency;
        out.duration_ms = ev.duration_ms;
        out.label = ev.label.data();
        out.midi_note = ev.midi_note;
        out.velocity = ev.velocity;
        out.is_rest = ev.is_rest;
    } catch (...) {
        return somnia_c_note_event_t{};
    }

    return out;
}

bool somnia_export_midi(const char* filepath,
                        uint32_t seed,
                        uint8_t scale_type,
                        int8_t root_offset,
                        uint16_t bpm,
                        size_t steps) {
    if (!filepath || steps == 0 || steps > MAX_EXPORT_STEPS) {
        return false;
    }

    try {
        ScaleEngine scaleEngine;
        PatternEngine patternEngine;
        SequenceEngine sequenceEngine(scaleEngine, patternEngine);
        StreamState state{};

        ScaleType scale = static_cast<ScaleType>(scale_type % 10);
        uint8_t norm_root = safeNormalizeRoot(root_offset);

        std::vector<NoteEvent> sequence;
        sequence.reserve(steps);

        for (size_t i = 0; i < steps; ++i) {
            sequence.push_back(
                sequenceEngine.nextEvent(state, 0.25f, scale, seed, norm_root, bpm, true));
        }

        return MidiEngine::exportToFile(filepath, sequence, bpm);
    } catch (...) {
        return false;
    }
}

bool somnia_export_wav(const char* filepath,
                       uint32_t seed,
                       uint8_t scale_type,
                       int8_t root_offset,
                       uint16_t bpm,
                       size_t steps) {
    if (!filepath || steps == 0 || steps > MAX_EXPORT_STEPS) {
        return false;
    }

    try {
        ScaleEngine scaleEngine;
        PatternEngine patternEngine;
        SequenceEngine sequenceEngine(scaleEngine, patternEngine);
        StreamState state{};

        ScaleType scale = static_cast<ScaleType>(scale_type % 10);
        uint8_t norm_root = safeNormalizeRoot(root_offset);

        std::vector<NoteEvent> sequence;
        sequence.reserve(steps);

        for (size_t i = 0; i < steps; ++i) {
            sequence.push_back(
                sequenceEngine.nextEvent(state, 0.25f, scale, seed, norm_root, bpm, true));
        }

        return WavEngine::exportToFile(filepath, sequence, 44100);
    } catch (...) {
        return false;
    }
}

const char* somnia_version(void) {
    return "0.6.0";
}

} // extern "C"
