#pragma once

#include <stdint.h>

#include "somnia_compat.hpp"

/**
 * @brief Pitch descriptor containing western notation label, frequency in Hz, and MIDI note.
 */
struct Note {
    std_compat::string_view name; /**< Western pitch label (e.g. "C4", "G#5"). */
    uint16_t frequency = 0;       /**< 12-TET tuned fundamental frequency in Hz. */
    uint8_t midi_note = 60;       /**< Standard MIDI pitch number (0..127). */
    bool is_rest = false;         /**< True if this note represents a silence. */

    constexpr Note() = default;
    constexpr Note(std_compat::string_view n, uint16_t f, uint8_t m, bool r = false)
        : name(n), frequency(f), midi_note(m), is_rest(r) {
    }
};

/**
 * @brief Supported modal musical scales and pentatonic systems.
 */
enum class ScaleType : uint8_t {
    Lydian = 0,      /**< Major with augmented 4th (1, 2, 3, #4, 5, 6, 7). */
    MinorPentatonic, /**< Pentatonic minor (1, b3, 4, 5, b7). */
    MajorPentatonic, /**< Pentatonic major (1, 2, 3, 5, 6). */
    Dorian,          /**< Minor with natural 6th (1, 2, b3, 4, 5, 6, b7). */
    NaturalMinor,    /**< Aeolian minor (1, 2, b3, 4, 5, b6, b7). */
    Major,           /**< Ionian major (1, 2, 3, 4, 5, 6, 7). */
    Mixolydian,      /**< Major with dominant 7th (1, 2, 3, 4, 5, 6, b7). */
    Phrygian,        /**< Minor with flat 2nd (1, b2, b3, 4, 5, b6, b7). */
    Blues,           /**< Hexatonic blues with flatted 5th (1, b3, 4, b5, 5, b7). */
    Hirajoshi        /**< Traditional Japanese folk pentatonic (1, 2, b3, 5, b6). */
};

/**
 * @brief 12-TET tuning system and musical scale quantization engine.
 *
 * Provides instant O(1) note lookups using precalculated frequency tables
 * and PROGMEM storage for resource-constrained AVR microcontrollers.
 */
class ScaleEngine {
public:
    ScaleEngine() = default;

    /**
     * @brief Quantizes a scale step index to an absolute tuned musical note.
     *
     * @param index       Sequential scale step index.
     * @param scale       Target musical mode.
     * @param rootOffset  Semitone transposition offset from C (0..11).
     * @param baseOctave  Base octave register (default: 4).
     * @return Tuned Note descriptor.
     */
    [[nodiscard]] Note
    getNote(size_t index, ScaleType scale, uint8_t rootOffset = 0, int baseOctave = 4) const;

    /**
     * @brief Backward-compatibility convenience overload.
     */
    [[nodiscard]] Note getNote(size_t index, bool useLydian) const {
        return getNote(index, useLydian ? ScaleType::Lydian : ScaleType::MinorPentatonic, 0, 4);
    }

    /**
     * @brief Parses a string name into a ScaleType enum.
     */
    static ScaleType parseScale(std_compat::string_view name);

    /**
     * @brief Checks if a string name is a valid scale name.
     */
    static bool isValidScaleName(std_compat::string_view name);

    /**
     * @brief Returns the human-readable canonical string name of a scale.
     */
    static std_compat::string_view scaleName(ScaleType scale);

    /**
     * @brief Parses a note name (e.g. "C", "F#", "Bb") to a chromatic semitone offset (0..11).
     */
    static uint8_t parseRootNote(std_compat::string_view name);

    /**
     * @brief Checks if a string name is a valid musical note name (e.g. "C", "F#", "Bb").
     */
    static bool isValidRootNote(std_compat::string_view name);

    /**
     * @brief Returns the note name string for a chromatic semitone offset (0..11).
     */
    static std_compat::string_view rootNoteName(uint8_t rootOffset);

    /**
     * @brief Retrieves the 12-TET tuned fundamental frequency in Hz for a MIDI note index (0..127).
     *
     * @param midiNote MIDI note index.
     * @return Fundamental frequency in Hz rounded to nearest integer.
     */
    static uint16_t getFrequencyForMidi(uint8_t midiNote);

private:
    static constexpr std_compat::array<int, 7> m_lydianIntervals = {0, 2, 4, 6, 7, 9, 11};
    static constexpr std_compat::array<int, 5> m_minorPentatonicIntervals = {0, 3, 5, 7, 10};
    static constexpr std_compat::array<int, 5> m_majorPentatonicIntervals = {0, 2, 4, 7, 9};
    static constexpr std_compat::array<int, 7> m_dorianIntervals = {0, 2, 3, 5, 7, 9, 10};
    static constexpr std_compat::array<int, 7> m_naturalMinorIntervals = {0, 2, 3, 5, 7, 8, 10};
    static constexpr std_compat::array<int, 7> m_majorIntervals = {0, 2, 4, 5, 7, 9, 11};
    static constexpr std_compat::array<int, 7> m_mixolydianIntervals = {0, 2, 4, 5, 7, 9, 10};
    static constexpr std_compat::array<int, 7> m_phrygianIntervals = {0, 1, 3, 5, 7, 8, 10};
    static constexpr std_compat::array<int, 6> m_bluesIntervals = {0, 3, 5, 6, 7, 10};
    static constexpr std_compat::array<int, 5> m_hirajoshiIntervals = {0, 2, 3, 7, 8};

    static constexpr std_compat::array<std_compat::string_view, 12> m_noteNames =
        {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};
