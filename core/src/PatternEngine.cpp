#include "PatternEngine.hpp"

#include "SomniaCompat.hpp"

#if __cplusplus < 201703L
constexpr std_compat::array<uint8_t, 256> PatternEngine::m_permutation;
#endif

float PatternEngine::getSample(float t, uint32_t seed) const {
    float t_pos = t < 0.0f ? 0.0f : t;

    int i0 = static_cast<int>(std::floor(t_pos));
    int i1 = i0 + 1;
    float f0 = t_pos - static_cast<float>(i0);
    float f1 = f0 - 1.0f;

    // Deterministically scramble/hash indexes using seed and bitwise operation
    size_t hashed_i0 = (static_cast<size_t>(i0) ^ seed) & 255;
    size_t hashed_i1 = (static_cast<size_t>(i1) ^ seed) & 255;

    uint8_t p0 = SOMNIA_READ_BYTE(&m_permutation[hashed_i0]);
    uint8_t p1 = SOMNIA_READ_BYTE(&m_permutation[hashed_i1]);

    float g0 = (p0 & 1) ? 1.0f : -1.0f;
    float g1 = (p1 & 1) ? 1.0f : -1.0f;

    float v0 = g0 * f0;
    float v1 = g1 * f1;

    // Hermite cubic S-curve fade
    float s = f0 * f0 * (3.0f - 2.0f * f0);

    float noise = std_compat::lerp(v0, v1, s);
    return noise + 0.5f; // Normalize [-0.5, 0.5] range to [0.0, 1.0]
}

float PatternEngine::getFbmSample(float t, uint32_t seed, int octaves) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_val = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        float sample = getSample(t * frequency, seed) - 0.5f;
        result += sample * amplitude;
        max_val += 0.5f * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    if (max_val == 0.0f) {
        return 0.5f;
    }
    return (result / max_val) * 0.5f + 0.5f;
}

uint8_t PatternEngine::getSampleQ8(int32_t t_q8, uint32_t seed) const {
    if (t_q8 < 0) {
        t_q8 = 0;
    }
    int32_t i0 = t_q8 >> 8;
    int32_t f0 = t_q8 & 255;
    int32_t f1 = f0 - 256;

    size_t hashed_i0 = (static_cast<size_t>(i0) ^ seed) & 255;
    size_t hashed_i1 = (static_cast<size_t>(i0 + 1) ^ seed) & 255;

    uint8_t p0 = SOMNIA_READ_BYTE(&m_permutation[hashed_i0]);
    uint8_t p1 = SOMNIA_READ_BYTE(&m_permutation[hashed_i1]);

    int32_t g0 = (p0 & 1) ? 1 : -1;
    int32_t g1 = (p1 & 1) ? 1 : -1;

    int32_t v0 = g0 * f0;
    int32_t v1 = g1 * f1;

    // Hermite cubic S-curve in Q8: s = (f0^2 * (768 - 2*f0)) >> 16
    int32_t s = (f0 * f0 * (768 - 2 * f0)) >> 16;
    if (s < 0) {
        s = 0;
    } else if (s > 256) {
        s = 256;
    }

    int32_t noise = v0 + ((s * (v1 - v0)) >> 8);
    int32_t norm = (noise >> 1) + 128;
    if (norm < 0) {
        norm = 0;
    } else if (norm > 255) {
        norm = 255;
    }
    return static_cast<uint8_t>(norm);
}

uint8_t PatternEngine::getFbmSampleQ8(int32_t t_q8, uint32_t seed, int octaves) const {
    int32_t result = 0;
    int32_t amp = 128;
    int32_t freq_mult = 1;
    int32_t max_val = 0;

    for (int i = 0; i < octaves; ++i) {
        int32_t sample = static_cast<int32_t>(getSampleQ8(t_q8 * freq_mult, seed)) - 128;
        result += (sample * amp) >> 7;
        max_val += amp;
        amp >>= 1;
        freq_mult <<= 1;
    }

    if (max_val == 0) {
        return 128;
    }
    int32_t normalized = (result * 128) / max_val + 128;
    if (normalized < 0) {
        normalized = 0;
    } else if (normalized > 255) {
        normalized = 255;
    }
    return static_cast<uint8_t>(normalized);
}
