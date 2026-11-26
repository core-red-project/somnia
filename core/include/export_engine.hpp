#pragma once

#include <string>
#include <vector>

#include "sequence_engine.hpp"
#include "somnia_compat.hpp"

class ExportEngine {
public:
    // Retained for backward compatibility
    [[deprecated("Use toJson() instead")]] [[nodiscard]] std::string
    toMockJson(std_compat::span<const NoteEvent> sequence) const;
    [[deprecated("Use toJson() instead")]] [[nodiscard]] std::string
    toMockJson(const std::vector<NoteEvent>& sequence) const;

    // Standard modern formats
    [[nodiscard]] std::string toJson(std_compat::span<const NoteEvent> sequence) const;
    [[nodiscard]] std::string toJson(const std::vector<NoteEvent>& sequence) const {
        return toJson(std_compat::span<const NoteEvent>(sequence.data(), sequence.size()));
    }

    [[nodiscard]] std::string toJsonLines(std_compat::span<const NoteEvent> sequence) const;
    [[nodiscard]] std::string toJsonLines(const std::vector<NoteEvent>& sequence) const {
        return toJsonLines(std_compat::span<const NoteEvent>(sequence.data(), sequence.size()));
    }

    [[nodiscard]] std::string toCsv(std_compat::span<const NoteEvent> sequence) const;
    [[nodiscard]] std::string toCsv(const std::vector<NoteEvent>& sequence) const {
        return toCsv(std_compat::span<const NoteEvent>(sequence.data(), sequence.size()));
    }
};
