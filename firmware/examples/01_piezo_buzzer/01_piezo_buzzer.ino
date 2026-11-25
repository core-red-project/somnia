// Somnia Example 01: Piezo Buzzer Player
// Ultra-lightweight deterministic procedural audio stream for Arduino Uno / Nano (ATmega328P).
// Zero dynamic allocations, O(1) memory, zero floating-point operations.

#include <Arduino.h>
#include <Somnia.h>

// Hardware Pin Definition
constexpr uint8_t BUZZER_PIN = 8;

ScaleEngine scaleEngine;
PatternEngine patternEngine;
SequenceEngine sequenceEngine(scaleEngine, patternEngine);
BuzzerAudioOutput audio(BUZZER_PIN);

static uint32_t activeSeed = 1337;
static StreamStateQ8 state{};
constexpr uint16_t bpm = 120;
constexpr ScaleType activeScale = ScaleType::MinorPentatonic;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial console
    }
    Serial.println(F("✦ Somnia Example 01: Piezo Buzzer Initialized"));
}

void loop() {
    uint32_t currentMillis = ::millis();
    audio.update(currentMillis);

    if (audio.isBusy()) {
        return; // Non-blocking cooperative polling
    }

    static uint32_t lastNoteEndTime = 0;
    static bool resting = false;
    constexpr uint32_t restDuration = 50; // 50ms pause between notes

    if (resting) {
        if (currentMillis - lastNoteEndTime < restDuration) {
            return;
        }
        resting = false;
    }

    // 0.25 time step in Q8.8 fixed-point (256 * 0.25 = 64)
    constexpr int32_t timeStepQ8 = 64;

    NoteEvent event =
        sequenceEngine.nextEventQ8(state, timeStepQ8, activeScale, activeSeed, 0, bpm, true);

    if (!event.is_rest && event.frequency > 0) {
        Serial.print(F("Note: "));
        Serial.print(event.label.data());
        Serial.print(F(" | Freq: "));
        Serial.print(event.frequency);
        Serial.print(F(" Hz | Dur: "));
        Serial.print(event.duration_ms);
        Serial.println(F(" ms"));

        audio.playTone(event.frequency, event.duration_ms, event.velocity);
    }

    lastNoteEndTime = ::millis();
    resting = true;
}
