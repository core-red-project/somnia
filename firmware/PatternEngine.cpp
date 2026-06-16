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

    float g0 = (m_permutation[hashed_i0] & 1) ? 1.0f : -1.0f;
    float g1 = (m_permutation[hashed_i1] & 1) ? 1.0f : -1.0f;

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
