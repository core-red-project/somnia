#pragma once

#include "SequenceEngine.hpp"

#include <string>
#include <vector>

class ExportEngine {
public:
    std::string toMockJson(const std::vector<NoteEvent>& sequence) const;
};
