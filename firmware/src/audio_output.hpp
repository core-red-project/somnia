#pragma once

#ifdef ARDUINO
#include <stdint.h>
#else
#include <cstdint>
#endif

// Audio HAL Event structure representing an audio emission command
struct AudioToneEvent {
    uint16_t frequency = 0;   // Tone frequency in Hz (0 = rest/silence)
    uint16_t duration_ms = 0; // Duration in milliseconds
    uint8_t velocity = 100;   // Dynamic intensity/volume (0..127)
    uint8_t midi_note = 60;   // MIDI note number (0..127)
    bool is_rest = false;     // True if note represents a musical rest
};

// Pure virtual Audio HAL Interface
// Decouples procedural sequence engines from hardware synthesis backends
class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    // Direct tone playback with velocity/volume support
    virtual void playTone(uint16_t frequency, uint16_t duration_ms, uint8_t velocity = 100) = 0;

    // Structured event playback
    virtual void playEvent(const AudioToneEvent& event) {
        if (event.is_rest || event.frequency == 0) {
            stop();
        } else {
            playTone(event.frequency, event.duration_ms, event.velocity);
        }
    }

    // Stop tone generation immediately
    virtual void stop() = 0;

    // Non-blocking cooperative state update (called in loop())
    virtual void update(uint32_t currentMillis) = 0;

    // Check if the backend is actively rendering a note
    virtual bool isBusy() const = 0;
};
