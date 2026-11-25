// Somnia Example 02: Native USB-MIDI Streamer
// Class-compliant USB-MIDI output with 24 PPQN MIDI Clock generator.
// Compatible with RP2040 (Raspberry Pi Pico) and ESP32-S2/S3 via Adafruit TinyUSB.

#include <Arduino.h>
#include <Somnia.h>

#if defined(ARDUINO_ARCH_RP2040) || defined(ESP32)
UsbMidiOutput midiOut;
#else
// Fallback to Serial UART MIDI for standard AVR boards
SerialMidiOutput midiOut(Serial);
#endif

MidiClockGenerator midiClock;
ScaleEngine scaleEngine;
PatternEngine patternEngine;
SequenceEngine sequenceEngine(scaleEngine, patternEngine);

static uint32_t activeSeed = 424242;
static StreamStateQ8 state{};
constexpr uint16_t bpm = 110;
constexpr ScaleType activeScale = ScaleType::Dorian;

void setup() {
    midiOut.begin();
    midiClock.setBpm(bpm);
    midiClock.start(midiOut, ::micros());
}

void loop() {
    uint32_t currentMicros = ::micros();
    uint32_t currentMillis = ::millis();

    // 24 PPQN real-time clock pulses for DAW tempo sync
    midiClock.update(midiOut, currentMicros);

    static uint32_t lastNoteTime = 0;
    static uint32_t currentNoteDuration = 250;
    static uint8_t playingNote = 0;
    static bool noteActive = false;

    // Send Note Off when note duration elapses
    if (noteActive && (currentMillis - lastNoteTime >= currentNoteDuration)) {
        midiOut.noteOff(playingNote);
        midiOut.flush();
        noteActive = false;
    }

    // Step every quarter note (500ms at 120 BPM, ~545ms at 110 BPM)
    constexpr uint32_t stepIntervalMs = 60000UL / (bpm * 2); // 8th note resolution
    if (currentMillis - lastNoteTime < stepIntervalMs) {
        return;
    }

    constexpr int32_t timeStepQ8 = 128; // 0.5 step in Q8
    NoteEvent event =
        sequenceEngine.nextEventQ8(state, timeStepQ8, activeScale, activeSeed, 0, bpm, true);

    lastNoteTime = currentMillis;

    if (!event.is_rest && event.midi_note > 0) {
        playingNote = event.midi_note;
        currentNoteDuration = (event.duration_ms * 4) / 5; // 80% gate length
        midiOut.noteOn(playingNote, event.velocity);
        midiOut.flush();
        noteActive = true;
    }
}
