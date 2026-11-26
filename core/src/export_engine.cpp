#include "export_engine.hpp"

#include <sstream>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

std::string ExportEngine::toMockJson(std_compat::span<const NoteEvent> sequence) const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto& event = sequence[i];
        oss << "{\"note\":\"" << event.label.data() << "\",\"duration\":" << event.duration_ms
            << "}";
        if (i + 1 < sequence.size()) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}

std::string ExportEngine::toMockJson(const std::vector<NoteEvent>& sequence) const {
    return toMockJson(std_compat::span<const NoteEvent>(sequence.data(), sequence.size()));
}

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

std::string ExportEngine::toJson(std_compat::span<const NoteEvent> sequence) const {
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto& event = sequence[i];
        oss << "  {\n"
            << "    \"index\": " << i << ",\n"
            << "    \"label\": \"" << event.label.data() << "\",\n"
            << "    \"midi\": " << static_cast<int>(event.midi_note) << ",\n"
            << "    \"frequency\": " << event.frequency << ",\n"
            << "    \"duration_ms\": " << event.duration_ms << ",\n"
            << "    \"is_rest\": " << (event.is_rest ? "true" : "false") << "\n"
            << "  }";
        if (i + 1 < sequence.size()) {
            oss << ",";
        }
        oss << "\n";
    }
    oss << "]\n";
    return oss.str();
}

std::string ExportEngine::toJsonLines(std_compat::span<const NoteEvent> sequence) const {
    std::ostringstream oss;
    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto& event = sequence[i];
        oss << "{\"index\":" << i << ",\"label\":\"" << event.label.data() << "\""
            << ",\"midi\":" << static_cast<int>(event.midi_note)
            << ",\"frequency\":" << event.frequency << ",\"duration_ms\":" << event.duration_ms
            << ",\"is_rest\":" << (event.is_rest ? "true" : "false") << "}\n";
    }
    return oss.str();
}

std::string ExportEngine::toCsv(std_compat::span<const NoteEvent> sequence) const {
    std::ostringstream oss;
    oss << "index,label,midi,frequency_hz,duration_ms,is_rest\n";
    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto& event = sequence[i];
        oss << i << "," << event.label.data() << "," << static_cast<int>(event.midi_note) << ","
            << event.frequency << "," << event.duration_ms << "," << (event.is_rest ? 1 : 0)
            << "\n";
    }
    return oss.str();
}
