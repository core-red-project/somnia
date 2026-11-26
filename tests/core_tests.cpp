#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

#include "../cli/include/cli_args.hpp"
#include "../firmware/analog_filter.hpp"
#include "../firmware/somnia_midi.hpp"
#include "../firmware/somnia_storage.hpp"
#include "adsr_envelope.hpp"
#include "export_engine.hpp"
#include "midi_engine.hpp"
#include "pattern_engine.hpp"
#include "polyphonic_mixer.hpp"
#include "scale_engine.hpp"
#include "sequence_engine.hpp"
#include "somnia_c.h"
#include "wav_engine.hpp"

#define ASSERT_TRUE(condition, message)                                                            \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::cerr << "Assertion failed: " << message << " at " << __FILE__ << ":" << __LINE__  \
                      << std::endl;                                                                \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)

void testScaleEngineBounds() {
    ScaleEngine scaleEngine;

    Note note1 = scaleEngine.getNote(9999, false);
    Note note2 = scaleEngine.getNote(9999, true);

    ASSERT_TRUE(!note1.name.empty(), "ScaleEngine (Pentatonic) returned empty note name");
    ASSERT_TRUE(note1.frequency > 0, "ScaleEngine (Pentatonic) returned 0Hz frequency");
    ASSERT_TRUE(!note2.name.empty(), "ScaleEngine (Lydian) returned empty note name");
    ASSERT_TRUE(note2.frequency > 0, "ScaleEngine (Lydian) returned 0Hz frequency");

    std::cout << "testScaleEngineBounds passed." << std::endl;
}

void testAllScalesAndRoots() {
    ScaleEngine scaleEngine;

    static const ScaleType scales[] = {ScaleType::Lydian,
                                       ScaleType::MinorPentatonic,
                                       ScaleType::MajorPentatonic,
                                       ScaleType::Dorian,
                                       ScaleType::NaturalMinor,
                                       ScaleType::Major,
                                       ScaleType::Mixolydian,
                                       ScaleType::Phrygian,
                                       ScaleType::Blues,
                                       ScaleType::Hirajoshi};

    for (ScaleType s : scales) {
        for (uint8_t root = 0; root < 12; ++root) {
            for (size_t i = 0; i < 20; ++i) {
                Note n = scaleEngine.getNote(i, s, root, 4);
                ASSERT_TRUE(!n.name.empty(), "Scale returned empty note name");
                ASSERT_TRUE(n.frequency > 0, "Scale returned 0Hz frequency");
                ASSERT_TRUE(n.midi_note >= 12 && n.midi_note <= 127, "MIDI note out of bounds");
            }
        }
    }

    std::cout << "testAllScalesAndRoots passed." << std::endl;
}

void testPatternEngineNormalization() {
    PatternEngine patternEngine;
    constexpr uint32_t seed = 42;

    for (int i = 0; i < 600; ++i) {
        float t = static_cast<float>(i) * 0.05f;
        float sample = patternEngine.getSample(t, seed);
        ASSERT_TRUE(sample >= 0.0f && sample <= 1.0f,
                    "PatternEngine sample value is not normalized within [0.0f, 1.0f] range");
    }

    std::cout << "testPatternEngineNormalization passed." << std::endl;
}

void testFixedPointQ8() {
    PatternEngine patternEngine;
    constexpr uint32_t seed = 777;

    for (int i = 0; i < 500; ++i) {
        int32_t t_q8 = (i * 256) / 20; // Step by ~0.05 in Q8
        uint8_t sample = patternEngine.getSampleQ8(t_q8, seed);
        uint8_t fbm = patternEngine.getFbmSampleQ8(t_q8, seed, 2);

        // Verification of deterministic integer boundaries
        ASSERT_TRUE(sample <= 255, "Q8 sample out of bounds");
        ASSERT_TRUE(fbm <= 255, "Q8 FBM sample out of bounds");
    }

    std::cout << "testFixedPointQ8 passed." << std::endl;
}

void testSequenceEngineQ8() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    StreamStateQ8 stateQ8{};
    constexpr int32_t timeStepQ8 = 64; // 0.25 in Q8
    constexpr uint32_t seed = 12345;

    for (size_t i = 0; i < 100; ++i) {
        NoteEvent ev = sequenceEngine.nextEventQ8(
            stateQ8, timeStepQ8, ScaleType::Lydian, seed, 0, 120, true, PlayMode::Melody);

        ASSERT_TRUE(ev.duration_ms > 0, "Q8 duration must be positive");
        ASSERT_TRUE(ev.velocity >= 40 && ev.velocity <= 127, "Q8 velocity out of range");
        if (!ev.is_rest) {
            ASSERT_TRUE(ev.frequency > 0, "Non-rest note must have non-zero frequency");
            ASSERT_TRUE(ev.midi_note >= 12 && ev.midi_note <= 127, "MIDI note out of range");
        }
    }

    std::cout << "testSequenceEngineQ8 passed." << std::endl;
}

void testSequenceEngineIntegrity() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    constexpr size_t steps = 16;
    constexpr float timeStep = 0.25f;
    constexpr uint32_t seed = 999;

    StreamState state{};

    for (size_t i = 0; i < steps; ++i) {
        NoteEvent event = sequenceEngine.nextEvent(state, timeStep, true, seed);
        ASSERT_TRUE(event.frequency > 0, "Event contains invalid or uninitialized frequency");
        ASSERT_TRUE(!event.label.empty(), "Event contains empty note label");
        ASSERT_TRUE(event.duration_ms == 100 || event.duration_ms == 200 ||
                        event.duration_ms == 400,
                    "Event duration is out of procedural bounds");
    }

    ASSERT_TRUE(state.step == steps, "State step count does not match execution limit");

    std::cout << "testSequenceEngineIntegrity passed." << std::endl;
}

void testPlayModesAndTimeSignatures() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    StreamState state{};
    constexpr uint32_t seed = 1234;

    // Test Arpeggio mode
    for (size_t i = 0; i < 8; ++i) {
        NoteEvent ev = sequenceEngine.nextEvent(state,
                                                0.25f,
                                                ScaleType::Lydian,
                                                seed,
                                                0,
                                                120,
                                                false,
                                                PlayMode::Arpeggio,
                                                TimeSignature::ThreeFour);
        ASSERT_TRUE(ev.frequency > 0, "Arpeggio generated 0 Hz");
        ASSERT_TRUE(ev.velocity >= 40 && ev.velocity <= 127, "Velocity out of bounds");
    }

    std::cout << "testPlayModesAndTimeSignatures passed." << std::endl;
}

void testDeterminism() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    constexpr size_t steps = 32;
    constexpr float timeStep = 0.25f;
    constexpr uint32_t seed = 888777;

    StreamState state1{};
    StreamState state2{};

    for (size_t i = 0; i < steps; ++i) {
        NoteEvent ev1 =
            sequenceEngine.nextEvent(state1, timeStep, ScaleType::Dorian, seed, 2, 120, true);
        NoteEvent ev2 =
            sequenceEngine.nextEvent(state2, timeStep, ScaleType::Dorian, seed, 2, 120, true);

        ASSERT_TRUE(ev1.frequency == ev2.frequency, "Non-deterministic frequency detected");
        ASSERT_TRUE(ev1.duration_ms == ev2.duration_ms, "Non-deterministic duration detected");
        ASSERT_TRUE(ev1.midi_note == ev2.midi_note, "Non-deterministic midi note detected");
        ASSERT_TRUE(ev1.velocity == ev2.velocity, "Non-deterministic velocity detected");
        ASSERT_TRUE(ev1.is_rest == ev2.is_rest, "Non-deterministic rest state detected");
    }

    // Golden vector validation for seed 888777 (Dorian, root 2 (D), 120 bpm, no rests)
    StreamState goldenState{};
    static const uint8_t expectedMidi[8] = {100, 69, 108, 108, 100, 62, 89, 62};
    static const uint16_t expectedFreq[8] = {2637, 440, 4186, 4186, 2637, 294, 1397, 294};
    static const uint8_t expectedVel[8] = {83, 75, 83, 91, 83, 75, 83, 91};

    for (size_t i = 0; i < 8; ++i) {
        NoteEvent ev =
            sequenceEngine.nextEvent(goldenState, timeStep, ScaleType::Dorian, seed, 2, 120, false);
        ASSERT_TRUE(ev.midi_note == expectedMidi[i], "Golden MIDI note mismatch at step");
        ASSERT_TRUE(ev.frequency == expectedFreq[i], "Golden frequency mismatch at step");
        ASSERT_TRUE(ev.velocity == expectedVel[i], "Golden velocity mismatch at step");
    }

    std::cout << "testDeterminism passed." << std::endl;
}

void testMidiExport() {
    std::vector<NoteEvent> sequence = {NoteEvent(440, 250, "A4", 69, 100, false),
                                       NoteEvent(523, 500, "C5", 72, 95, false),
                                       NoteEvent(0, 250, "REST", 0, 0, true),
                                       NoteEvent(659, 1000, "E5", 76, 110, false)};

    auto midiData = MidiEngine::generateMidiData(sequence, 120, TimeSignature::ThreeFour);
    ASSERT_TRUE(midiData.size() >= 22, "MIDI data too small to contain header and track");

    ASSERT_TRUE(midiData[0] == 'M' && midiData[1] == 'T' && midiData[2] == 'h' &&
                    midiData[3] == 'd',
                "Invalid MIDI MThd chunk identifier");

    // Check for MTrk chunk
    bool foundMTrk = false;
    for (size_t i = 0; i + 4 <= midiData.size(); ++i) {
        if (midiData[i] == 'M' && midiData[i + 1] == 'T' && midiData[i + 2] == 'r' &&
            midiData[i + 3] == 'k') {
            foundMTrk = true;
            break;
        }
    }
    ASSERT_TRUE(foundMTrk, "Missing MTrk chunk in generated MIDI stream");

    // Check for Time Signature meta event (FF 58 04 nn dd cc bb)
    // For 3/4 time signature: nn = 3, dd = 2 (2^2 = 4)
    bool foundTimeSig = false;
    for (size_t i = 0; i + 6 < midiData.size(); ++i) {
        if (midiData[i] == 0xFF && midiData[i + 1] == 0x58 && midiData[i + 2] == 0x04) {
            ASSERT_TRUE(midiData[i + 3] == 3, "Expected numerator 3 for 3/4 meter");
            ASSERT_TRUE(midiData[i + 4] == 2,
                        "Expected denominator 2 (quarter note) for 3/4 meter");
            foundTimeSig = true;
            break;
        }
    }
    ASSERT_TRUE(foundTimeSig, "Missing Time Signature meta event in MIDI stream");

    // Check for Note On event for A4 (MIDI note 69, status 0x90)
    bool foundNoteOn = false;
    for (size_t i = 0; i + 2 < midiData.size(); ++i) {
        if (midiData[i] == 0x90 && midiData[i + 1] == 69) {
            foundNoteOn = true;
            break;
        }
    }
    ASSERT_TRUE(foundNoteOn, "Missing Note On event for MIDI note 69 in stream");

    std::cout << "testMidiExport passed." << std::endl;
}

void testWavExport() {
    std::vector<NoteEvent> sequence = {NoteEvent(440, 100, "A4", 69, 100, false),
                                       NoteEvent(880, 200, "A5", 81, 90, false)};

    const uint32_t sampleRate = 44100;
    auto wavData = WavEngine::generateWavData(sequence, sampleRate);
    ASSERT_TRUE(wavData.size() > 44, "WAV data too small for RIFF header");

    ASSERT_TRUE(wavData[0] == 'R' && wavData[1] == 'I' && wavData[2] == 'F' && wavData[3] == 'F',
                "Invalid WAV RIFF identifier");
    ASSERT_TRUE(wavData[8] == 'W' && wavData[9] == 'A' && wavData[10] == 'V' && wavData[11] == 'E',
                "Invalid WAV WAVE format identifier");

    // Check "fmt " subchunk
    ASSERT_TRUE(wavData[12] == 'f' && wavData[13] == 'm' && wavData[14] == 't' &&
                    wavData[15] == ' ',
                "Missing 'fmt ' chunk in WAV");

    // Check audio format is 1 (PCM)
    uint16_t audioFormat =
        static_cast<uint16_t>(wavData[20]) | (static_cast<uint16_t>(wavData[21]) << 8);
    ASSERT_TRUE(audioFormat == 1, "Expected PCM audio format (1)");

    // Check num channels (1 = mono)
    uint16_t numChannels =
        static_cast<uint16_t>(wavData[22]) | (static_cast<uint16_t>(wavData[23]) << 8);
    ASSERT_TRUE(numChannels == 1, "Expected mono (1 channel)");

    // Check sample rate matches 44100
    uint32_t parsedRate =
        static_cast<uint32_t>(wavData[24]) | (static_cast<uint32_t>(wavData[25]) << 8) |
        (static_cast<uint32_t>(wavData[26]) << 16) | (static_cast<uint32_t>(wavData[27]) << 24);
    ASSERT_TRUE(parsedRate == sampleRate, "Parsed sample rate does not match 44100");

    // Check "data" subchunk
    bool foundData = false;
    for (size_t i = 12; i + 4 <= wavData.size(); ++i) {
        if (wavData[i] == 'd' && wavData[i + 1] == 'a' && wavData[i + 2] == 't' &&
            wavData[i + 3] == 'a') {
            foundData = true;
            // Verify data size roughly matches 300ms of 16-bit audio: 0.3 * 44100 * 2 = ~26460
            // bytes
            uint32_t dataBytes = static_cast<uint32_t>(wavData[i + 4]) |
                                 (static_cast<uint32_t>(wavData[i + 5]) << 8) |
                                 (static_cast<uint32_t>(wavData[i + 6]) << 16) |
                                 (static_cast<uint32_t>(wavData[i + 7]) << 24);
            ASSERT_TRUE(dataBytes >= 20000, "WAV data payload unexpectedly small");
            break;
        }
    }
    ASSERT_TRUE(foundData, "Missing 'data' chunk in WAV header");

    std::cout << "testWavExport passed." << std::endl;
}

void testExportEngineFormats() {
    ExportEngine exporter;
    std::vector<NoteEvent> sequence = {NoteEvent(440, 250, "A4", 69, 100, false)};

    std::string json = exporter.toJson(sequence);
    ASSERT_TRUE(json.find("\"label\": \"A4\"") != std::string::npos, "JSON output missing label");
    ASSERT_TRUE(json.find("\"frequency\": 440") != std::string::npos,
                "JSON output missing frequency");

    std::string jsonl = exporter.toJsonLines(sequence);
    ASSERT_TRUE(jsonl.find("\"frequency\":440") != std::string::npos,
                "JSONLines output missing frequency");

    std::string csv = exporter.toCsv(sequence);
    ASSERT_TRUE(csv.find("index,label,midi,frequency_hz") != std::string::npos,
                "CSV header missing");
    ASSERT_TRUE(csv.find("0,A4,69,440,250,0") != std::string::npos, "CSV data line incorrect");

    std::cout << "testExportEngineFormats passed." << std::endl;
}

void testCApi() {
    somnia_engine_t engine = somnia_create();
    ASSERT_TRUE(engine != nullptr, "somnia_create returned null handle");

    somnia_c_note_event_t ev = somnia_next_event(engine, 1234, 0, 0, 120, false, 0, 0);
    ASSERT_TRUE(ev.frequency > 0, "C-API returned invalid frequency");
    ASSERT_TRUE(ev.duration_ms > 0, "C-API returned 0 duration");
    ASSERT_TRUE(std::string(somnia_version()) == "0.6.0", "C-API version mismatch");

    // Test negative root offset does not overflow or crash
    somnia_c_note_event_t ev_neg = somnia_next_event(engine, 1234, 0, -1, 120, false, 0, 0);
    ASSERT_TRUE(ev_neg.frequency > 0, "C-API negative root returned invalid frequency");

    // Test safe export to MIDI and WAV
    bool midiOk = somnia_export_midi("test_c_export.mid", 1234, 0, 0, 120, 8);
    ASSERT_TRUE(midiOk, "somnia_export_midi failed");

    bool wavOk = somnia_export_wav("test_c_export.wav", 1234, 0, 0, 120, 8);
    ASSERT_TRUE(wavOk, "somnia_export_wav failed");

    // Test bounds and length_error protection
    bool nullFileOk = somnia_export_midi(nullptr, 1234, 0, 0, 120, 8);
    ASSERT_TRUE(!nullFileOk, "somnia_export_midi with null filepath must return false");

    bool zeroStepsOk = somnia_export_midi("test_zero.mid", 1234, 0, 0, 120, 0);
    ASSERT_TRUE(!zeroStepsOk, "somnia_export_midi with 0 steps must return false");

    // Enormous step count must return false instead of throwing std::length_error
    bool overflowOk = somnia_export_midi("overflow.mid", 1234, 0, 0, 120, 1ULL << 60);
    ASSERT_TRUE(!overflowOk, "somnia_export_midi with 1<<60 steps must return false safely");

    // Clean up temporary files
    std::remove("test_c_export.mid");
    std::remove("test_c_export.wav");

    somnia_destroy(engine);
    std::cout << "testCApi passed." << std::endl;
}

void testAdsrAndPolyphony() {
    AdsrEnvelope env;
    env.noteOn(127);
    ASSERT_TRUE(env.isActive(), "Envelope should be active after noteOn");

    // Advance through attack (15ms)
    env.advance(15);
    ASSERT_TRUE(env.getLevel() > 0, "Level should be non-zero after attack");

    // Advance through decay (35ms) to sustain
    env.advance(35);
    ASSERT_TRUE(env.getState() == EnvelopeState::Sustain, "State should be Sustain");

    // Release note
    env.noteOff();
    ASSERT_TRUE(env.getState() == EnvelopeState::Release, "State should be Release");

    // Advance through release (60ms)
    env.advance(60);
    ASSERT_TRUE(!env.isActive(), "Envelope should be idle after full release");
    ASSERT_TRUE(env.getLevel() == 0, "Level should be 0 after release");

    // Test 4-Voice Polyphonic Mixer
    PolyphonicMixer<4> mixer;
    mixer.noteOn(440, 100); // A4
    mixer.noteOn(554, 100); // C#5
    mixer.noteOn(659, 100); // E5
    ASSERT_TRUE(mixer.getActiveVoiceCount() == 3, "Mixer should have 3 active voices");

    mixer.advanceEnvelopes(10);
    int16_t sample = mixer.renderMixedSample(44100);
    (void)sample;

    mixer.allNotesOff();
    mixer.advanceEnvelopes(100);
    ASSERT_TRUE(mixer.getActiveVoiceCount() == 0, "All voices should be idle after release");

    std::cout << "testAdsrAndPolyphony passed." << std::endl;
}

class MockMidiOutput : public MidiOutput {
public:
    void begin() override {
        initialized = true;
    }
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        (void)channel;
        lastNote = note;
        lastVelocity = velocity;
        noteOnCount++;
    }
    void noteOff(uint8_t note, uint8_t channel = 0) override {
        (void)channel;
        lastNote = note;
        noteOffCount++;
    }
    void controlChange(uint8_t controller, uint8_t value, uint8_t channel = 0) override {
        (void)controller;
        (void)value;
        (void)channel;
    }
    void sendClockTick() override {
        clockTickCount++;
    }
    void sendStart() override {
        started = true;
    }
    void sendStop() override {
        stopped = true;
    }
    void sendContinue() override {
    }
    void flush() override {
    }

    bool initialized = false;
    bool started = false;
    bool stopped = false;
    uint32_t clockTickCount = 0;
    uint32_t noteOnCount = 0;
    uint32_t noteOffCount = 0;
    uint8_t lastNote = 0;
    uint8_t lastVelocity = 0;
};

void testMidiOutputAndClock() {
    MockMidiOutput mock;
    mock.begin();
    ASSERT_TRUE(mock.initialized, "Mock MIDI output should be initialized");

    mock.noteOn(60, 100);
    ASSERT_TRUE(mock.noteOnCount == 1, "Expected noteOn count 1");
    ASSERT_TRUE(mock.lastNote == 60, "Expected note 60");
    ASSERT_TRUE(mock.lastVelocity == 100, "Expected velocity 100");

    mock.noteOff(60);
    ASSERT_TRUE(mock.noteOffCount == 1, "Expected noteOff count 1");

    MidiClockGenerator clock;
    clock.setBpm(120);
    clock.start(mock, 0);

    ASSERT_TRUE(mock.started, "MIDI Start should be sent on clock start");
    ASSERT_TRUE(clock.isRunning(), "Clock should be running");

    // At 120 BPM, tick interval is 2500000 / 120 = 20833 us
    // Advance time through 24 ticks (1 quarter note = 500,000 us)
    for (uint32_t t = 20833; t <= 500000; t += 20833) {
        clock.update(mock, t);
    }

    ASSERT_TRUE(mock.clockTickCount >= 24,
                "Should have received at least 24 clock ticks for 1 quarter note");
    ASSERT_TRUE(clock.getTickCount() >= 24, "Clock internal tick counter should be at least 24");

    clock.stop(mock);
    ASSERT_TRUE(mock.stopped, "MIDI Stop should be sent on clock stop");
    ASSERT_TRUE(!clock.isRunning(), "Clock should be stopped");

    std::cout << "testMidiOutputAndClock passed." << std::endl;
}

void testAnalogFilterAndHysteresis() {
    // 1. Test EmaFilter
    EmaFilter<3> filter(0); // Shift = 3 -> alpha = 1/8
    ASSERT_TRUE(!filter.isInitialized(), "Filter should start uninitialized when created with 0");

    // First update should snap to input value
    uint16_t firstVal = filter.update(500);
    ASSERT_TRUE(firstVal == 500, "Initial update should snap directly to raw value");
    ASSERT_TRUE(filter.isInitialized(), "Filter should be initialized after first update");

    // Feed a step to 1000 and verify smoothing behavior
    uint16_t prev = firstVal;
    for (int i = 0; i < 20; ++i) {
        uint16_t current = filter.update(1000);
        ASSERT_TRUE(current >= prev, "Filter step response must be monotonically increasing");
        ASSERT_TRUE(current <= 1000, "Filter output cannot overshoot target on low-pass filter");
        prev = current;
    }
    // After 20 cycles, it should be very close to 1000 (e.g. > 900)
    ASSERT_TRUE(prev > 900, "Filter should converge towards target step");

    // Test noise rejection: rapid +/- 10 fluctuation around 500
    filter.reset(500);
    for (int i = 0; i < 10; ++i) {
        uint16_t noisyInput = (i % 2 == 0) ? 510 : 490;
        uint16_t out = filter.update(noisyInput);
        // Output should stay tightly centered around 500
        int diff = std::abs(static_cast<int>(out) - 500);
        ASSERT_TRUE(diff < 5, "EMA filter should reject high-frequency alternating noise");
    }

    // 2. Test HysteresisSelector
    // 10 states across 0..1023 -> bucketSize = 1024 / 10 = 102
    // Hysteresis deadband = 12
    HysteresisSelector<10, 1023, 12> selector;
    ASSERT_TRUE(!selector.isInitialized(), "Selector should start uninitialized");

    // First reading at 50 should start in state 0
    uint8_t state0 = selector.update(50);
    ASSERT_TRUE(state0 == 0, "50 should map to state 0");

    // Threshold between state 0 and 1 is 102.
    // Upward transition requires >= 102 + 12 = 114.
    // Reading 105 is past 102 but below 114 -> should stay in state 0!
    ASSERT_TRUE(selector.update(105) == 0, "105 must stay in state 0 due to hysteresis deadband");
    ASSERT_TRUE(selector.update(113) == 0, "113 must stay in state 0 due to hysteresis deadband");

    // Reading 115 is >= 114 -> transitions to state 1
    ASSERT_TRUE(selector.update(115) == 1, "115 must transition upward to state 1");

    // Now in state 1, downward threshold is 102 - 12 = 90.
    // Reading 100 is below 102, but above 90 -> must STAY in state 1!
    ASSERT_TRUE(selector.update(100) == 1, "100 must stay in state 1 due to hysteresis deadband");
    ASSERT_TRUE(selector.update(92) == 1, "92 must stay in state 1 due to hysteresis deadband");

    // Reading 88 is <= 90 -> transitions back down to state 0
    ASSERT_TRUE(selector.update(88) == 0, "88 must transition downward back to state 0");

    std::cout << "testAnalogFilterAndHysteresis passed." << std::endl;
}

void testSomniaStorage() {
    // 1. Verify schema constants and defaults
    SomniaConfig cfg;
    cfg.updateChecksum();
    ASSERT_TRUE(cfg.isValid(), "Default SomniaConfig should be valid after updateChecksum()");
    ASSERT_TRUE(cfg.magic == SomniaConfig::MAGIC, "Config magic should match 0x534F4D4E");
    ASSERT_TRUE(cfg.version == SomniaConfig::SCHEMA_VERSION, "Config version should match 1");

    // 2. Verify corruption detection
    SomniaConfig corrupted = cfg;
    corrupted.savedSeed = 999999; // mutate without updating checksum
    ASSERT_TRUE(!corrupted.isValid(), "Corrupted config must fail validation");

    corrupted = cfg;
    corrupted.magic = 0x12345678;
    ASSERT_TRUE(!corrupted.isValid(), "Invalid magic header must fail validation");

    // 3. Test StorageManager with MemoryStorageBackend
    MemoryStorageBackend<128> backend;
    StorageManager manager(backend);

    // Initial uninitialized storage (all 0xFF) must fail to load
    SomniaConfig loaded{};
    bool loadedOk = manager.load(loaded);
    ASSERT_TRUE(!loadedOk, "Uninitialized EEPROM memory must fail to load");

    // Save custom configuration
    cfg.savedSeed = 0xDEADBEEF;
    cfg.defaultBpm = 145;
    cfg.defaultScale = 7;
    bool saveOk = manager.save(cfg);
    ASSERT_TRUE(saveOk, "Saving valid config to backend must succeed");

    // Load back and verify all fields match
    SomniaConfig restored{};
    loadedOk = manager.load(restored);
    ASSERT_TRUE(loadedOk, "Loading previously saved config must succeed");
    ASSERT_TRUE(restored.savedSeed == 0xDEADBEEF, "Restored seed must match 0xDEADBEEF");
    ASSERT_TRUE(restored.defaultBpm == 145, "Restored BPM must match 145");
    ASSERT_TRUE(restored.defaultScale == 7, "Restored scale must match 7");
    ASSERT_TRUE(restored.isValid(), "Restored config must pass validation");

    std::cout << "testSomniaStorage passed." << std::endl;
}

void testConformanceFixtures() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    // Read and verify against conformance/scales_conformance.json if present
    std::ifstream jsonFile("conformance/scales_conformance.json");
    if (!jsonFile.is_open()) {
        jsonFile.open("../conformance/scales_conformance.json");
    }
    if (jsonFile.is_open()) {
        std::stringstream buffer;
        buffer << jsonFile.rdbuf();
        std::string jsonStr = buffer.str();
        ASSERT_TRUE(jsonStr.find("\"version\": \"0.6.0\"") != std::string::npos,
                    "Conformance JSON must be version 0.6.0");
        ASSERT_TRUE(jsonStr.find("[98, 79, 87, 60]") != std::string::npos,
                    "Conformance JSON missing expected dorian notes");
        ASSERT_TRUE(jsonStr.find("[93, 88, 76, 95]") != std::string::npos,
                    "Conformance JSON missing expected hirajoshi notes");
    }

    // Test 1: C Dorian (seed 12345, 120 bpm, 4 steps, root 0, no rests)
    StreamState state1{};
    std::vector<uint8_t> actualDorian;
    for (size_t i = 0; i < 4; ++i) {
        NoteEvent ev =
            sequenceEngine.nextEvent(state1, 0.25f, ScaleType::Dorian, 12345, 0, 120, false);
        actualDorian.push_back(ev.midi_note);
    }
    std::vector<uint8_t> expectedDorian = {98, 79, 87, 60};
    ASSERT_TRUE(actualDorian == expectedDorian,
                "Dorian conformance test failed to match expected golden MIDI notes");

    // Test 2: Hirajoshi (seed 42, 100 bpm, 4 steps, root 9, no rests)
    StreamState state2{};
    std::vector<uint8_t> actualHirajoshi;
    for (size_t i = 0; i < 4; ++i) {
        NoteEvent ev =
            sequenceEngine.nextEvent(state2, 0.25f, ScaleType::Hirajoshi, 42, 9, 100, false);
        actualHirajoshi.push_back(ev.midi_note);
    }
    std::vector<uint8_t> expectedHirajoshi = {93, 88, 76, 95};
    ASSERT_TRUE(actualHirajoshi == expectedHirajoshi,
                "Hirajoshi conformance test failed to match expected golden MIDI notes");

    std::cout << "testConformanceFixtures passed." << std::endl;
}

void test32BitNonPeriodicity() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    // Verify 32-bit seeds produce distinct outputs (seed 0 vs 256)
    StreamState stateA{}, stateB{};
    std::vector<uint8_t> seq0, seq256;
    for (size_t i = 0; i < 4; ++i) {
        seq0.push_back(
            sequenceEngine.nextEvent(stateA, 0.25f, ScaleType::Dorian, 0, 0, 120, false).midi_note);
        seq256.push_back(
            sequenceEngine.nextEvent(stateB, 0.25f, ScaleType::Dorian, 256, 0, 120, false)
                .midi_note);
    }
    ASSERT_TRUE(seq0 != seq256,
                "Seed 0 and 256 must not produce identical sequences under 32-bit hash mixer");

    // Verify step i and step i+1024 are non-periodic
    StreamState stateRun{};
    std::vector<uint8_t> first1024;
    for (size_t i = 0; i < 1024; ++i) {
        NoteEvent ev =
            sequenceEngine.nextEvent(stateRun, 0.25f, ScaleType::Dorian, 42, 0, 120, false);
        first1024.push_back(ev.midi_note);
    }
    std::vector<uint8_t> next1024;
    for (size_t i = 0; i < 1024; ++i) {
        NoteEvent ev =
            sequenceEngine.nextEvent(stateRun, 0.25f, ScaleType::Dorian, 42, 0, 120, false);
        next1024.push_back(ev.midi_note);
    }
    ASSERT_TRUE(first1024 != next1024,
                "Melody stream must not repeat identically every 1024 steps");

    std::cout << "test32BitNonPeriodicity passed." << std::endl;
}

void testLongStreamStability() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    // Simulate stream far into the future (past 2^22 = 4,194,304 steps)
    StreamState state{};
    state.cell = 4194304;
    state.frac = 0.0f;
    state.step = 4194304;

    NoteEvent ev1 = sequenceEngine.nextEvent(state, 0.25f, ScaleType::Lydian, 777, 0, 120, false);
    ASSERT_TRUE(ev1.duration_ms > 0, "Long stream event 1 must have valid duration");
    ASSERT_TRUE(state.cell == 4194304, "Cell offset tracking error");
    ASSERT_TRUE(state.frac == 0.25f, "Frac progression error at large cell index");

    NoteEvent ev2 = sequenceEngine.nextEvent(state, 0.25f, ScaleType::Lydian, 777, 0, 120, false);
    ASSERT_TRUE(ev2.duration_ms > 0, "Long stream event 2 must have valid duration");
    ASSERT_TRUE(state.frac == 0.50f, "Frac progression error at large cell index");

    NoteEvent ev3 = sequenceEngine.nextEvent(state, 0.25f, ScaleType::Lydian, 777, 0, 120, false);
    NoteEvent ev4 = sequenceEngine.nextEvent(state, 0.25f, ScaleType::Lydian, 777, 0, 120, false);
    ASSERT_TRUE(ev3.duration_ms > 0 && ev4.duration_ms > 0, "Long stream events must be valid");
    ASSERT_TRUE(state.cell == 4194305, "Cell must increment after 1.0 phase accumulated");
    ASSERT_TRUE(state.frac == 0.0f, "Frac must wrap modulo 1.0");

    // Also test Q8 long stream does not overflow signed integers
    StreamStateQ8 stateQ8{};
    stateQ8.time_q8 = 0x7FFFF000; // Near int32 positive boundary
    NoteEvent evQ8 = sequenceEngine.nextEventQ8(stateQ8, 64, ScaleType::Lydian, 777, 0, 120, false);
    ASSERT_TRUE(evQ8.duration_ms > 0, "Q8 long stream produced invalid duration");

    std::cout << "testLongStreamStability passed." << std::endl;
}

void testCliArgumentValidation() {
    // 1. Unknown flag must throw invalid_argument
    {
        const char* argv[] = {"somnia", "--sclae", "dorian"};
        bool caught = false;
        try {
            parseCommandLine(3, const_cast<char**>(argv));
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        ASSERT_TRUE(caught, "Cli parser failed to reject unknown flag '--sclae'");
    }

    // 2. Unknown scale name must throw
    {
        const char* argv[] = {"somnia", "--scale", "dorain"};
        bool caught = false;
        try {
            parseCommandLine(3, const_cast<char**>(argv));
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        ASSERT_TRUE(caught, "Cli parser failed to reject typo in scale name 'dorain'");
    }

    // 3. Invalid root note must throw
    {
        const char* argv[] = {"somnia", "--root", "H"};
        bool caught = false;
        try {
            parseCommandLine(3, const_cast<char**>(argv));
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        ASSERT_TRUE(caught, "Cli parser failed to reject invalid root note 'H'");
    }

    // 4. Invalid BPM (0 or non-numeric) must throw
    {
        const char* argv[] = {"somnia", "--bpm", "0"};
        bool caught = false;
        try {
            parseCommandLine(3, const_cast<char**>(argv));
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        ASSERT_TRUE(caught, "Cli parser failed to reject BPM = 0");
    }

    // 5. Valid command parsing
    {
        const char* argv[] = {"somnia",
                              "export",
                              "-o",
                              "custom.mid",
                              "-s",
                              "dorian",
                              "-r",
                              "D",
                              "-b",
                              "140",
                              "-n",
                              "32"};
        CliConfig cfg = parseCommandLine(12, const_cast<char**>(argv));
        ASSERT_TRUE(cfg.command == "export", "Failed to parse command 'export'");
        ASSERT_TRUE(cfg.outputFile == "custom.mid", "Failed to parse output file");
        ASSERT_TRUE(cfg.scaleName == "dorian", "Failed to parse scale");
        ASSERT_TRUE(cfg.rootNote == "D", "Failed to parse root note");
        ASSERT_TRUE(cfg.bpm == 140, "Failed to parse BPM");
        ASSERT_TRUE(cfg.steps == 32, "Failed to parse steps");
    }

    std::cout << "testCliArgumentValidation passed." << std::endl;
}

int main() {
    std::cout << "=== Running Core Unit Tests ===" << std::endl;
    testScaleEngineBounds();
    testAllScalesAndRoots();
    testPatternEngineNormalization();
    testFixedPointQ8();
    testSequenceEngineQ8();
    testSequenceEngineIntegrity();
    testPlayModesAndTimeSignatures();
    testDeterminism();
    testMidiExport();
    testWavExport();
    testExportEngineFormats();
    testAdsrAndPolyphony();
    testMidiOutputAndClock();
    testAnalogFilterAndHysteresis();
    testSomniaStorage();
    testConformanceFixtures();
    test32BitNonPeriodicity();
    testLongStreamStability();
    testCApi();
    testCliArgumentValidation();
    std::cout << "=================================" << std::endl;
    std::cout << "All unit tests completed successfully." << std::endl;
    return 0;
}
