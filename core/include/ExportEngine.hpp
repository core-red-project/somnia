#pragma once

#include "SequenceEngine.hpp"

#include <string>
#include <vector>

class ExportEngine {
public:
    // Retained for backward compatibility
    std::string toMockJson(const std::vector<NoteEvent>& sequence) const;

    // Standard modern formats
    std::string toJson(const std::vector<NoteEvent>& sequence) const;
    std::string toJsonLines(const std::vector<NoteEvent>& sequence) const;
    std::string toCsv(const std::vector<NoteEvent>& sequence) const;
};
