#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SequenceEngine.hpp"

#include <iostream>

int main() {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    constexpr size_t steps = 16;
    constexpr float timeStep = 0.25f;
    constexpr uint32_t seed = 12345;

    StreamState state{};

    for (size_t i = 0; i < steps; ++i) {
        // Evaluate the next immediate event on-the-fly (O(1) memory model)
        NoteEvent event = sequenceEngine.nextEvent(state, timeStep, true, seed);

        // Output matching verbatim: [LABEL] ([FREQUENCY]Hz) -> [DURATION]ms
        std::cout << event.label << " (" << event.frequency << "Hz) -> " << event.duration_ms
                  << "ms" << std::endl;
    }

    return 0;
}
