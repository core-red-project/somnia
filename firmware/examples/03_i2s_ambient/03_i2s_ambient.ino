// Somnia Example 03: I2S Polyphonic Ambient Engine
// High-fidelity 16-bit 44.1 kHz stereo audio streaming with virtual polyphony and ADSR envelopes.
// Designed for ESP32 and RP2040 connected to an external I2S DAC (MAX98357A, PCM5102A).

#include <Arduino.h>
#include <Somnia.h>

// Default I2S Pins (ESP32 Standard: BCLK=26, LRC=25, DOUT=22)
constexpr int I2S_BCLK_PIN = 26;
constexpr int I2S_LRC_PIN = 25;
constexpr int I2S_DOUT_PIN = 22;

I2sAudioOutput audio(I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
ScaleEngine scaleEngine;
PatternEngine patternEngine;
SequenceEngine sequenceEngine(scaleEngine, patternEngine);

static uint32_t activeSeed = 987654;
static StreamStateQ8 state{};
constexpr uint16_t bpm = 80;
constexpr ScaleType activeScale = ScaleType::Lydian;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port
    }

    Serial.println(F("✦ Somnia Example 03: I2S Polyphonic Ambient Engine"));
    audio.begin();

    // Configure warm ambient ADSR envelope (Attack: 100ms, Decay: 200ms, Sustain: 70%, Release:
    // 500ms)
    audio.getMixer().setGlobalEnvelope(100, 200, 179, 500);
}

void loop() {
    uint32_t currentMillis = ::millis();
    audio.update(currentMillis);

    static uint32_t lastStepTime = 0;
    constexpr uint32_t stepIntervalMs = 60000UL / (bpm * 2);

    if (currentMillis - lastStepTime < stepIntervalMs) {
        return;
    }
    lastStepTime = currentMillis;

    constexpr int32_t timeStepQ8 = 64; // 0.25 Q8
    NoteEvent event =
        sequenceEngine.nextEventQ8(state, timeStepQ8, activeScale, activeSeed, 0, bpm, true);

    if (!event.is_rest && event.frequency > 0) {
        audio.playTone(event.frequency, event.duration_ms, event.velocity);
        Serial.print(F("Polyphonic Voice Note: "));
        Serial.print(event.label.data());
        Serial.print(F(" | Active Voices: "));
        Serial.println(audio.getMixer().getActiveVoiceCount());
    }
}
