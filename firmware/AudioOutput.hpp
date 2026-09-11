#pragma once

#ifdef ARDUINO
#include <stdint.h>
#else
#include <cstdint>
#endif

class AudioOutput {
public:
    virtual ~AudioOutput() = default;

    virtual void playTone(uint16_t frequency, uint16_t duration_ms) = 0;
    virtual void update(uint32_t currentMillis) = 0;
    virtual bool isBusy() const = 0;
};
