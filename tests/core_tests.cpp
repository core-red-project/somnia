#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SequenceEngine.hpp"

#include <cmath>
#include <cstdlib>
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

    // Test extreme out-of-bounds lookup index
    Note note1 = scaleEngine.getNote(9999, false);
    Note note2 = scaleEngine.getNote(9999, true);

    ASSERT_TRUE(!note1.name.empty(), "ScaleEngine (Pentatonic) returned empty note name");
    ASSERT_TRUE(note1.frequency > 0, "ScaleEngine (Pentatonic) returned 0Hz frequency");
    ASSERT_TRUE(!note2.name.empty(), "ScaleEngine (Lydian) returned empty note name");
    ASSERT_TRUE(note2.frequency > 0, "ScaleEngine (Lydian) returned 0Hz frequency");

    std::cout << "testScaleEngineBounds passed." << std::endl;
}

void testPatternEngineNormalization() {
    PatternEngine patternEngine;
    constexpr uint32_t seed = 42;

    // Loop over 500 iterative samples
    for (int i = 0; i < 600; ++i) {
        float t = static_cast<float>(i) * 0.05f;
        float sample = patternEngine.getSample(t, seed);
        ASSERT_TRUE(sample >= 0.0f && sample <= 1.0f,
                    "PatternEngine sample value is not normalized within [0.0f, 1.0f] range");
    }

    std::cout << "testPatternEngineNormalization passed." << std::endl;
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

int main() {
    std::cout << "=== Running Core Unit Tests ===" << std::endl;
    testScaleEngineBounds();
    testPatternEngineNormalization();
    testSequenceEngineIntegrity();
    std::cout << "=================================" << std::endl;
    std::cout << "All unit tests completed successfully." << std::endl;
    return 0;
}
