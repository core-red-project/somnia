#include <Arduino.h>

#include "src/Somnia.h"

// Hardware Pin Configuration
#if defined(ESP32)
const uint8_t BUZZER_PIN = 25;      // DAC1 / GPIO 25
const uint8_t MUTATION_BTN_PIN = 4; // GPIO 4
const uint8_t POT_TEMPO_PIN = 34;   // ADC1_CH6
const uint8_t POT_SCALE_PIN = 35;   // ADC1_CH7
const uint8_t POT_SEED_PIN = 32;    // ADC1_CH4
#elif defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO)
const uint8_t BUZZER_PIN = 15;      // GPIO 15
const uint8_t MUTATION_BTN_PIN = 2; // GPIO 2
const uint8_t POT_TEMPO_PIN = A0;   // GPIO 26 / ADC0
const uint8_t POT_SCALE_PIN = A1;   // GPIO 27 / ADC1
const uint8_t POT_SEED_PIN = A2;    // GPIO 28 / ADC2
#else // Standard AVR (Arduino Uno / Nano)
const uint8_t BUZZER_PIN = 8;
const uint8_t MUTATION_BTN_PIN = 2; // External hardware interrupt pin for live seed mutation
const uint8_t POT_TEMPO_PIN = A0;   // Analog pot for tempo (BPM)
const uint8_t POT_SCALE_PIN = A1;   // Analog pot for scale selection
const uint8_t POT_SEED_PIN = A2;    // Analog pot for seed offset
#endif

// Enable hardware MIDI output over TX pin (31250 baud)
#define ENABLE_MIDI_SERIAL 0

ScaleEngine scaleEngine;
PatternEngine patternEngine;
SequenceEngine sequenceEngine(scaleEngine, patternEngine);
BuzzerAudioOutput audio(BUZZER_PIN);
SerialMidiOutput midiOut(Serial);
MidiClockGenerator midiClock;

// Physical analog input smoothing and hysteresis protection
static EmaFilter<3> tempoFilter;
static HysteresisSelector<10, 1023, 12> scaleSelector;

#ifdef ARDUINO
static EepromStorageBackend eepromBackend;
static StorageManager storage(eepromBackend);
#endif

static uint32_t activeSeed = 54321;
static StreamStateQ8 state{};
static uint16_t currentBpm = 120;
static ScaleType currentScale = ScaleType::MinorPentatonic;

// Soft takeover: preserve loaded EEPROM preset until the user physically turns the knobs
static bool presetBpmLocked = false;
static bool presetScaleLocked = false;
static uint16_t initialPotTempo = 0;
static uint16_t initialPotScale = 0;

void sendMidiNoteOn(uint8_t note, uint8_t velocity) {
#if ENABLE_MIDI_SERIAL
    midiOut.noteOn(note, velocity);
#endif
}

void sendMidiNoteOff(uint8_t note) {
#if ENABLE_MIDI_SERIAL
    midiOut.noteOff(note);
#endif
}

void setup() {
#if defined(ESP32)
    analogReadResolution(10); // Standardize ESP32 ADC to 10-bit (0..1023)
#endif

#if ENABLE_MIDI_SERIAL
    midiOut.begin();
    midiClock.start(midiOut, ::micros());
#else
    Serial.begin(115200);
    uint32_t serialStart = ::millis();
    // Non-blocking timeout: do not hang standalone Pico or ESP32-S2 powered without USB PC host
    while (!Serial && (::millis() - serialStart < 2500)) {
        ;
    }
    Serial.println(F("✦ SOMNIA HW-v0.6 Initialized"));
    Serial.println(F("Deterministic O(1) Procedural Sound Core"));
#endif

    // Setup mutation/preset button with internal pull-up
    pinMode(MUTATION_BTN_PIN, INPUT_PULLUP);

#ifdef ARDUINO
    eepromBackend.begin(sizeof(SomniaConfig));
    SomniaConfig loadedCfg{};
    if (storage.load(loadedCfg)) {
        activeSeed = loadedCfg.savedSeed;
        currentBpm = loadedCfg.defaultBpm;
        currentScale = static_cast<ScaleType>(loadedCfg.defaultScale);

        // Soft takeover: lock preset values until potentiometer knob turns past deadband
        presetBpmLocked = true;
        presetScaleLocked = true;
        initialPotTempo = analogRead(POT_TEMPO_PIN);
        initialPotScale = analogRead(POT_SCALE_PIN);

#if !ENABLE_MIDI_SERIAL
        Serial.print(F("✦ Restored Preset from EEPROM | Seed: "));
        Serial.print(activeSeed);
        Serial.print(F(" | BPM: "));
        Serial.print(currentBpm);
        Serial.print(F(" | Scale: "));
        Serial.println(static_cast<uint8_t>(currentScale));
#endif
    } else {
        // Randomize initial seed if analog noise pin is floating and no EEPROM preset
        activeSeed ^= (static_cast<uint32_t>(analogRead(POT_SEED_PIN)) << 8);
    }
#else
    activeSeed ^= (static_cast<uint32_t>(analogRead(POT_SEED_PIN)) << 8);
#endif
}

void loop() {
    uint32_t currentMillis = ::millis();

#if ENABLE_MIDI_SERIAL
    midiClock.update(midiOut, ::micros());
#endif

    // Debounced button state machine: short press = mutate seed, hold >= 1.5s = save preset to
    // EEPROM
    static bool lastBtnState = HIGH;
    static uint32_t btnPressTime = 0;
    static bool longPressHandled = false;

    bool btnState = digitalRead(MUTATION_BTN_PIN);
    if (lastBtnState == HIGH && btnState == LOW) {
        btnPressTime = currentMillis;
        longPressHandled = false;
    } else if (btnState == LOW && !longPressHandled) {
        if (currentMillis - btnPressTime >= 1500) {
#ifdef ARDUINO
            SomniaConfig cfg;
            cfg.savedSeed = activeSeed;
            cfg.defaultBpm = currentBpm;
            cfg.defaultScale = static_cast<uint8_t>(currentScale);
            storage.save(cfg);
#if !ENABLE_MIDI_SERIAL
            Serial.println(F("[PRESET SAVED TO EEPROM]"));
#endif
#endif
            longPressHandled = true;
        }
    } else if (lastBtnState == LOW && btnState == HIGH) {
        if (!longPressHandled && (currentMillis - btnPressTime > 40)) {
            // Short press (debounced): mutate active seed
            activeSeed = activeSeed * 1664525u + 1013904223u;
#if !ENABLE_MIDI_SERIAL
            Serial.print(F("[MUTATED SEED: "));
            Serial.print(activeSeed);
            Serial.println(F("]"));
#endif
        }
    }
    lastBtnState = btnState;

    // Stateful hardware polling loop
    audio.update(currentMillis);

    static uint8_t activeMidiNote = 0;
    static bool midiNoteActive = false;

    // Cooperative scheduling: if audio is still busy playing a note, continue polling
    if (audio.isBusy()) {
        return;
    }

    // Previous note has ended, send MIDI Note Off if active
    if (midiNoteActive) {
        sendMidiNoteOff(activeMidiNote);
        midiNoteActive = false;
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

    // Read potentiometer inputs with digital low-pass filtering and hysteresis protection
    uint16_t rawTempo = analogRead(POT_TEMPO_PIN);
    uint16_t rawScale = analogRead(POT_SCALE_PIN);

    // Soft takeover: preserve loaded EEPROM preset until knob is intentionally turned (>30 counts)
    if (presetBpmLocked) {
        int diff = static_cast<int>(rawTempo) - static_cast<int>(initialPotTempo);
        if (diff < -30 || diff > 30) {
            presetBpmLocked = false;
        }
    }
    if (!presetBpmLocked) {
        uint16_t filteredTempo = tempoFilter.update(rawTempo);
        uint16_t bpm = map(filteredTempo, 0, 1023, 60, 180);
        currentBpm = bpm;
    }

    static uint16_t lastBpm = 0;
    if (currentBpm != lastBpm) {
        lastBpm = currentBpm;
#if ENABLE_MIDI_SERIAL
        midiClock.setBpm(currentBpm);
#endif
    }

    if (presetScaleLocked) {
        int diff = static_cast<int>(rawScale) - static_cast<int>(initialPotScale);
        if (diff < -30 || diff > 30) {
            presetScaleLocked = false;
        }
    }
    if (!presetScaleLocked) {
        uint8_t scaleIndex = scaleSelector.update(rawScale);
        currentScale = static_cast<ScaleType>(scaleIndex);
    }

    // 0.25 in Q8.8 fixed-point (256 * 0.25 = 64)
    constexpr int32_t timeStepQ8 = 64;

    // Evaluate next note on-the-fly with constant O(1) memory and pure integer arithmetic
    NoteEvent event = sequenceEngine.nextEventQ8(
        state, timeStepQ8, currentScale, activeSeed, 0, currentBpm, true);

#if !ENABLE_MIDI_SERIAL
    Serial.print(F("NOTE: "));
    Serial.print(event.label.data());
    Serial.print(F(" | MIDI: "));
    Serial.print(event.midi_note);
    Serial.print(F(" | Freq: "));
    Serial.print(event.frequency);
    Serial.print(F("Hz | Dur: "));
    Serial.print(event.duration_ms);
    Serial.print(F("ms | Vel: "));
    Serial.print(event.velocity);
    Serial.print(F(" | BPM: "));
    Serial.println(currentBpm);
#endif

    if (!event.is_rest && event.frequency > 0) {
        audio.playTone(event.frequency, event.duration_ms);
        sendMidiNoteOn(event.midi_note, event.velocity);
        activeMidiNote = event.midi_note;
        midiNoteActive = true;
    } else {
        resting = true;
        lastNoteEndTime = currentMillis;
    }
}
