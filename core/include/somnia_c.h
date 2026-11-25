/**
 * @file somnia_c.h
 * @brief Stable C-ABI Foreign Function Interface for the Somnia Procedural Audio Engine.
 *
 * Provides a pure C API without C++ name mangling, allowing dynamic linking
 * and interoperability with Python (ctypes/CFFI), Rust, Zig, Go, and C# (.NET P/Invoke).
 */

#ifndef SOMNIA_C_H
#define SOMNIA_C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct somnia_c_note_event_t
 * @brief Discrete procedural musical event yielded by the sound core.
 */
typedef struct {
    uint16_t frequency;   /**< Fundamental frequency in Hertz (Hz). */
    uint16_t duration_ms; /**< Note duration in milliseconds. */
    const char* label;    /**< Western notation pitch label (e.g. "C4", "A#5", "REST"). */
    uint8_t midi_note;    /**< Standard MIDI note index (0..127). */
    uint8_t velocity;     /**< MIDI key-strike velocity dynamics (0..127). */
    bool is_rest;         /**< True if this event represents a musical pause/silence. */
} somnia_c_note_event_t;

/**
 * @typedef somnia_engine_t
 * @brief Opaque pointer handle to a heap-allocated Somnia procedural sequence engine.
 */
typedef void* somnia_engine_t;

/**
 * @brief Instantiates and initializes a new Somnia procedural engine instance.
 *
 * Allocates the sequence, scale, and pattern engines with zero runtime dependencies.
 *
 * @return Opaque handle to the initialized engine, or NULL on allocation failure.
 *
 * @note Must be released with somnia_destroy() when no longer needed to prevent memory leaks.
 */
somnia_engine_t somnia_create(void);

/**
 * @brief Frees and invalidates an engine instance previously created by somnia_create().
 *
 * @param engine Opaque handle returned by somnia_create(). Safe to call with NULL.
 */
void somnia_destroy(somnia_engine_t engine);

/**
 * @brief Evaluates and generates the next deterministic procedural musical event in O(1) time.
 *
 * Uses deterministic Perlin Value Noise with Fractal Brownian Motion and Q8.8 fixed-point
 * arithmetic to calculate the next melodic step on-the-fly.
 *
 * @param engine      Valid opaque engine handle returned by somnia_create().
 * @param seed        Deterministic seed integer (PRNG state anchor).
 * @param scale_type  Musical mode index (0: Lydian, 1: Minor Pentatonic, 2: Major Pentatonic,
 *                    3: Dorian, 4: Minor, 5: Major, 6: Mixolydian, 7: Phrygian, 8: Blues, 9: Hirajoshi).
 * @param root_offset Root note pitch chromatic shift in semitones (-12..+12).
 * @param bpm         Playback tempo in Beats Per Minute (20..300).
 * @param allow_rests If true, stochastically inserts musical rests based on noise thresholds.
 * @param play_mode   Melodic mode (0: Linear Melody, 1: Arpeggio, 2: Polyphonic Chords).
 * @param time_sig    Rhythmic signature (0: 4/4, 1: 3/4, 2: 7/8).
 * @return Initialized somnia_c_note_event_t struct containing frequency, duration, and MIDI note.
 */
somnia_c_note_event_t somnia_next_event(somnia_engine_t engine, uint32_t seed, uint8_t scale_type,
                                        int8_t root_offset, uint16_t bpm, bool allow_rests,
                                        uint8_t play_mode, uint8_t time_sig);

/**
 * @brief Serializes a procedural melody sequence directly into a Standard MIDI File (SMF format 0).
 *
 * @param filepath    Target output file path string on the local filesystem.
 * @param seed        Deterministic generation seed.
 * @param scale_type  Musical mode index (0..9).
 * @param root_offset Chromatic semitone shift.
 * @param bpm         Playback tempo (BPM).
 * @param steps       Number of sequential melodic events to evaluate and serialize.
 * @return True on successful file write, false on I/O error or invalid parameters.
 */
bool somnia_export_midi(const char* filepath, uint32_t seed, uint8_t scale_type,
                        int8_t root_offset, uint16_t bpm, size_t steps);

/**
 * @brief Synthesizes and serializes a procedural melody sequence into an uncompressed 16-bit 44.1kHz WAV file.
 *
 * @param filepath    Target output file path string on the local filesystem.
 * @param seed        Deterministic generation seed.
 * @param scale_type  Musical mode index (0..9).
 * @param root_offset Chromatic semitone shift.
 * @param bpm         Playback tempo (BPM).
 * @param steps       Number of sequential melodic events to synthesize.
 * @return True on successful file write, false on I/O error or invalid parameters.
 */
bool somnia_export_wav(const char* filepath, uint32_t seed, uint8_t scale_type, int8_t root_offset,
                       uint16_t bpm, size_t steps);

/**
 * @brief Returns the SemVer version string of the compiled Somnia engine.
 *
 * @return Constant null-terminated string (e.g. "0.6.0").
 */
const char* somnia_version(void);

#ifdef __cplusplus
}
#endif

#endif // SOMNIA_C_H
