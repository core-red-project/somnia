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

const uint8_t BUZZER_PIN = 8;

ScaleEngine scaleEngine;
PatternEngine patternEngine;
SequenceEngine sequenceEngine(scaleEngine, patternEngine);
ArduinoAudioOutput audio(BUZZER_PIN);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port connection
    }
    // Protected literal string using F() macro to save SRAM
    Serial.println(F("SOMNIA HW-v0.3 Initialized"));
}

void loop() {
    uint32_t currentMillis = ::millis();

    // Stateful hardware polling loop
    audio.update(currentMillis);

    // Cooperative scheduling: skip execution if buzzer is currently playing a note
    if (audio.isBusy()) {
        return;
    }

    static uint32_t lastNoteEndTime = 0;
    static bool resting = false;
    constexpr uint32_t restDuration = 100; // 100ms breathing rest

    if (resting) {
        if (currentMillis - lastNoteEndTime < restDuration) {
            return;
        }
        resting = false;
    }

    static StreamState state{};
    constexpr float timeStep = 0.25f;
    constexpr uint32_t seed = 54321;

    // Evaluate the next immediate event on-the-fly (O(1) memory model)
    NoteEvent event = sequenceEngine.nextEvent(state, timeStep, true, seed);

    // Jitter-free lightweight serial telemetry frames to prevent TX buffer blocking
    Serial.print(F("N:"));
    Serial.print(event.label.data());
    Serial.print(F(" F:"));
    Serial.println(event.frequency);

    // Trigger non-blocking output
    audio.playTone(event.frequency, event.duration_ms);

    lastNoteEndTime = ::millis();
    resting = true;
}
