#pragma once

#include <stdint.h>

#include "scale_engine.hpp"
#include "somnia_compat.hpp"

class PatternEngine;

/**
 * @brief Procedural playback modes defining melodic movement and voice leading.
 */
enum class PlayMode : uint8_t {
    Melody = 0, /**< Stepwise melodic line traversal. */
    Arpeggio,   /**< Multi-octave broken chord arpeggiation. */
    Chords      /**< Simultaneous harmonic dyads and clusters. */
};

/**
 * @brief Musical metric time signatures.
 */
enum class TimeSignature : uint8_t {
    FourFour = 0, /**< Common time 4/4 meter. */
    ThreeFour,    /**< Triple time 3/4 meter (waltz). */
    SevenEight    /**< Complex additive 7/8 meter. */
};

/**
 * @brief Discrete musical note event yielded by the procedural engine.
 */
struct NoteEvent {
    uint16_t frequency = 0;             /**< Fundamental frequency in Hertz (Hz). */
    uint16_t duration_ms = 0;           /**< Duration in milliseconds. */
    std_compat::string_view label = ""; /**< Western notation pitch label (e.g. "C4", "A#5"). */
    uint8_t midi_note = 60;             /**< Standard MIDI note index (0..127). */
    uint8_t velocity = 100;             /**< Strike velocity dynamic level (0..127). */
    bool is_rest = false;               /**< True if this event represents silence. */

    constexpr NoteEvent() = default;
    constexpr NoteEvent(uint16_t f,
                        uint16_t d,
                        std_compat::string_view l,
                        uint8_t m,
                        uint8_t v,
                        bool r = false)
        : frequency(f), duration_ms(d), label(l), midi_note(m), velocity(v), is_rest(r) {
    }
};

/**
 * @brief Floating-point stream continuity state for host/desktop engines.
 */
struct StreamState {
    size_t step = 0;   /**< Monotonic step counter. */
    uint32_t cell = 0; /**< Monotonic integer cell index for non-degrading long streams. */
    float frac = 0.0f; /**< Sub-cell phase fraction in [0.0, 1.0). */
    float time = 0.0f; /**< Accumulated phase time for inspection. */
};

/**
 * @brief Pure integer fixed-point state (Q8.8) for resource-constrained embedded microcontrollers.
 *
 * Eliminates runtime floating-point emulation on 8-bit AVR and improves deterministic guarantees.
 */
struct StreamStateQ8 {
    size_t step = 0;      /**< Monotonic step counter. */
    uint32_t time_q8 = 0; /**< Q8.8 fixed-point phase accumulator (1.0 = 256). */
};

/**
 * @brief Stateful procedural sequence engine evaluating deterministic melody streams.
 */
class SequenceEngine {
public:
    /**
     * @brief Constructs a sequence engine referencing pre-allocated scale and pattern engines.
     *
     * @param scaleEngine   Reference to active ScaleEngine.
     * @param patternEngine Reference to active PatternEngine.
     */
    SequenceEngine(const ScaleEngine& scaleEngine, const PatternEngine& patternEngine);

    /**
     * @brief Evaluates the next procedural note event using floating-point phase progression.
     *
     * @param state       In-out stream continuity state tracker.
     * @param timeStep    Phase increment per step.
     * @param scale       Target musical scale mode.
     * @param seed        Deterministic PRNG seed anchor.
     * @param rootOffset  Root note semitone chromatic offset.
     * @param bpm         Playback tempo in BPM.
     * @param allowRests  Whether to evaluate stochastic musical rests.
     * @param mode        Melodic movement mode.
     * @param timeSig     Rhythmic metric time signature.
     * @return Generated NoteEvent struct.
     */
    NoteEvent nextEvent(StreamState& state,
                        float timeStep,
                        ScaleType scale,
                        uint32_t seed,
                        uint8_t rootOffset = 0,
                        uint16_t bpm = 0,
                        bool allowRests = false,
                        PlayMode mode = PlayMode::Melody,
                        TimeSignature timeSig = TimeSignature::FourFour) const;

    /**
     * @brief Evaluates the next procedural note event using pure integer fixed-point Q8.8
     * arithmetic.
     *
     * Guaranteed O(1) time and zero dynamic heap allocations, optimized for embedded MCUs.
     *
     * @param state       In-out integer stream state tracker.
     * @param timeStepQ8  Q8.8 phase increment (256 = 1.0).
     * @param scale       Target musical scale mode.
     * @param seed        Deterministic PRNG seed anchor.
     * @param rootOffset  Root note semitone chromatic offset.
     * @param bpm         Playback tempo in BPM.
     * @param allowRests  Whether to evaluate stochastic musical rests.
     * @param mode        Melodic movement mode.
     * @param timeSig     Rhythmic metric time signature.
     * @return Generated NoteEvent struct.
     */
    NoteEvent nextEventQ8(StreamStateQ8& state,
                          int32_t timeStepQ8,
                          ScaleType scale,
                          uint32_t seed,
                          uint8_t rootOffset = 0,
                          uint16_t bpm = 0,
                          bool allowRests = false,
                          PlayMode mode = PlayMode::Melody,
                          TimeSignature timeSig = TimeSignature::FourFour) const;

    /**
     * @brief Backward-compatibility convenience overload.
     */
    NoteEvent nextEvent(StreamState& state, float timeStep, bool useLydian, uint32_t seed) const {
        return nextEvent(state,
                         timeStep,
                         useLydian ? ScaleType::Lydian : ScaleType::MinorPentatonic,
                         seed,
                         0,
                         0,
                         false,
                         PlayMode::Melody,
                         TimeSignature::FourFour);
    }

private:
    const ScaleEngine& m_scaleEngine;
    const PatternEngine& m_patternEngine;
};
