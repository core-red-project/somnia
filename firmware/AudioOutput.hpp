#pragma once

#include <cstdint>

class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    virtual void playTone(uint16_t frequency, uint16_t duration_ms) = 0;
    virtual void rest(uint16_t duration_ms) = 0;
};
