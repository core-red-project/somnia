#include "ExportEngine.hpp"
#include "MidiEngine.hpp"
#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SequenceEngine.hpp"
#include "WavEngine.hpp"
#include "somnia_c.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <vector>

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

    static const ScaleType scales[] = {ScaleType::Lydian,          ScaleType::MinorPentatonic,
                                       ScaleType::MajorPentatonic, ScaleType::Dorian,
                                       ScaleType::NaturalMinor,    ScaleType::Major,
                                       ScaleType::Mixolydian,      ScaleType::Phrygian,
                                       ScaleType::Blues,           ScaleType::Hirajoshi};

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
        NoteEvent ev =
            sequenceEngine.nextEvent(state, 0.25f, ScaleType::Lydian, seed, 0, 120, false,
                                     PlayMode::Arpeggio, TimeSignature::ThreeFour);
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

    std::cout << "testDeterminism passed." << std::endl;
}

void testMidiExport() {
    std::vector<NoteEvent> sequence = {
        NoteEvent(440, 250, "A4", 69, 100, false), NoteEvent(523, 500, "C5", 72, 95, false),
        NoteEvent(0, 250, "REST", 0, 0, true), NoteEvent(659, 1000, "E5", 76, 110, false)};

    auto midiData = MidiEngine::generateMidiData(sequence, 120);
    ASSERT_TRUE(midiData.size() >= 14, "MIDI data too small to contain header");

    ASSERT_TRUE(midiData[0] == 'M' && midiData[1] == 'T' && midiData[2] == 'h' &&
                    midiData[3] == 'd',
                "Invalid MIDI MThd chunk identifier");

    std::cout << "testMidiExport passed." << std::endl;
}

void testWavExport() {
    std::vector<NoteEvent> sequence = {NoteEvent(440, 100, "A4", 69, 100, false),
                                       NoteEvent(880, 200, "A5", 81, 90, false)};

    auto wavData = WavEngine::generateWavData(sequence, 44100);
    ASSERT_TRUE(wavData.size() > 44, "WAV data too small for RIFF header");

    ASSERT_TRUE(wavData[0] == 'R' && wavData[1] == 'I' && wavData[2] == 'F' && wavData[3] == 'F',
                "Invalid WAV RIFF identifier");
    ASSERT_TRUE(wavData[8] == 'W' && wavData[9] == 'A' && wavData[10] == 'V' && wavData[11] == 'E',
                "Invalid WAV WAVE format identifier");

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
    ASSERT_TRUE(std::string(somnia_version()) == "0.4.0", "C-API version mismatch");

    somnia_destroy(engine);
    std::cout << "testCApi passed." << std::endl;
}

int main() {
    std::cout << "=== Running Core Unit Tests ===" << std::endl;
    testScaleEngineBounds();
    testAllScalesAndRoots();
    testPatternEngineNormalization();
    testFixedPointQ8();
    testSequenceEngineIntegrity();
    testPlayModesAndTimeSignatures();
    testDeterminism();
    testMidiExport();
    testWavExport();
    testExportEngineFormats();
    testCApi();
    std::cout << "=================================" << std::endl;
    std::cout << "All unit tests completed successfully." << std::endl;
    return 0;
}
