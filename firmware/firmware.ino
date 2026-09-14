#include "ArduinoAudioOutput.hpp"

#ifndef ARDUINO
#include "../core/include/PatternEngine.hpp"
#include "../core/include/ScaleEngine.hpp"
#include "../core/include/SequenceEngine.hpp"
#else
#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SequenceEngine.hpp"
#endif

#include <Arduino.h>

// Hardware Pin Configuration
const uint8_t BUZZER_PIN = 8;
const uint8_t MUTATION_BTN_PIN = 2; // External hardware interrupt pin for live seed mutation
const uint8_t POT_TEMPO_PIN = A0;   // Analog pot for tempo (BPM)
const uint8_t POT_SCALE_PIN = A1;   // Analog pot for scale selection
const uint8_t POT_SEED_PIN = A2;    // Analog pot for seed offset

// Enable hardware MIDI output over TX pin (31250 baud)
#define ENABLE_MIDI_SERIAL 0

ScaleEngine scaleEngine;
PatternEngine patternEngine;
SequenceEngine sequenceEngine(scaleEngine, patternEngine);
ArduinoAudioOutput audio(BUZZER_PIN);

static uint32_t activeSeed = 54321;
static StreamState state{};
static volatile bool g_mutationTriggered = false;

void onMutationInterrupt() {
    g_mutationTriggered = true;
}

void sendMidiNoteOn(uint8_t note, uint8_t velocity) {
#if ENABLE_MIDI_SERIAL
    Serial.write(0x90);
    Serial.write(note & 0x7F);
    Serial.write(velocity & 0x7F);
#endif
}

void sendMidiNoteOff(uint8_t note) {
#if ENABLE_MIDI_SERIAL
    Serial.write(0x80);
    Serial.write(note & 0x7F);
    Serial.write(0x00);
#endif
}

void setup() {
#if ENABLE_MIDI_SERIAL
    Serial.begin(31250); // Standard MIDI baudrate
#else
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port connection
    }
    Serial.println(F("✦ SOMNIA HW-v0.5 Initialized"));
    Serial.println(F("Deterministic O(1) Procedural Sound Core"));
#endif

    // Setup mutation button with internal pull-up and hardware interrupt
    pinMode(MUTATION_BTN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MUTATION_BTN_PIN), onMutationInterrupt, FALLING);

    // Randomize initial seed if analog noise pin is floating
    activeSeed ^= (static_cast<uint32_t>(analogRead(POT_SEED_PIN)) << 8);
}

void loop() {
    uint32_t currentMillis = ::millis();

    // Check hardware mutation button
    if (g_mutationTriggered) {
        activeSeed = activeSeed * 1664525u + 1013904223u;
        g_mutationTriggered = false;
#if !ENABLE_MIDI_SERIAL
        Serial.println(F("[MUTATED SEED]"));
#endif
    }

    // Stateful hardware polling loop
    audio.update(currentMillis);

    // Cooperative scheduling: skip execution if buzzer is currently playing a note
    if (audio.isBusy()) {
        return;
    }

    static uint32_t lastNoteEndTime = 0;
    static bool resting = false;
    constexpr uint32_t restDuration = 60; // 60ms breathing space between notes

    if (resting) {
        if (currentMillis - lastNoteEndTime < restDuration) {
            return;
        }
        resting = false;
    }

    // Read potentiometer inputs for dynamic real-time physical control
    uint16_t potTempo = analogRead(POT_TEMPO_PIN);
    uint16_t potScale = analogRead(POT_SCALE_PIN);

    // Map tempo: 60 BPM to 180 BPM
    uint16_t bpm = map(potTempo, 0, 1023, 60, 180);

    // Map scale: 0 to 9 across available musical modes
    ScaleType activeScale = static_cast<ScaleType>(map(potScale, 0, 1023, 0, 9));

    constexpr float timeStep = 0.25f;

    // Evaluate next note on-the-fly with constant O(1) memory complexity
    NoteEvent event =
        sequenceEngine.nextEvent(state, timeStep, activeScale, activeSeed, 0, bpm, true);

#if !ENABLE_MIDI_SERIAL
    // Lightweight serial telemetry frames
    if (event.is_rest) {
        Serial.println(F("REST"));
    } else {
        Serial.print(F("N:"));
        Serial.print(event.label.data());
        Serial.print(F(" F:"));
        Serial.print(event.frequency);
        Serial.print(F(" D:"));
        Serial.println(event.duration_ms);
    }
#endif

    if (!event.is_rest && event.frequency > 0) {
        audio.playTone(event.frequency, event.duration_ms);
        sendMidiNoteOn(event.midi_note, event.velocity);
    }

    lastNoteEndTime = ::millis();
    resting = true;
}
