#include "ExportEngine.hpp"

#include "SequenceEngine.hpp"

#include <sstream>

std::string ExportEngine::toMockJson(const std::vector<NoteEvent>& sequence) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto& event = sequence[i];
        oss << "{\"note\":\"" << event.label << "\",\"duration\":" << event.duration_ms << "}";
        if (i + 1 < sequence.size()) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}
