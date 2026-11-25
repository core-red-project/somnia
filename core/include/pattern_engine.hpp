#pragma once

#include <stdint.h>

#include "somnia_compat.hpp"

class PatternEngine {
public:
    PatternEngine() = default;

    // Standard floating-point evaluation
    [[nodiscard]] float getSample(float t, uint32_t seed) const;
    [[nodiscard]] float getSample(uint32_t cell, float frac, uint32_t seed) const;
    [[nodiscard]] float getFbmSample(float t, uint32_t seed, int octaves = 2) const;
    [[nodiscard]] float
    getFbmSample(uint32_t cell, float frac, uint32_t seed, int octaves = 2) const;

    // Pure Integer Fixed-Point (Q8.8) evaluation for 8-bit AVR microcontrollers
    // Eliminates all floating-point math, std::floor, and software emulated FPU
    [[nodiscard]] uint8_t getSampleQ8(uint32_t t_q8, uint32_t seed) const;
    [[nodiscard]] uint8_t getSampleQ8(int32_t t_q8, uint32_t seed) const {
        return getSampleQ8(static_cast<uint32_t>(t_q8 < 0 ? 0 : t_q8), seed);
    }
    [[nodiscard]] uint8_t getFbmSampleQ8(uint32_t t_q8, uint32_t seed, int octaves = 2) const;
    [[nodiscard]] uint8_t getFbmSampleQ8(int32_t t_q8, uint32_t seed, int octaves = 2) const {
        return getFbmSampleQ8(static_cast<uint32_t>(t_q8 < 0 ? 0 : t_q8), seed, octaves);
    }

private:
    static const uint8_t m_permutation[256] SOMNIA_PROGMEM;
};
