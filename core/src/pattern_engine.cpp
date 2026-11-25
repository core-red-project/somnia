#include "pattern_engine.hpp"

#include "somnia_compat.hpp"

// Classic Perlin 256-entry permutation table placed in PROGMEM for embedded MCUs
const uint8_t PatternEngine::m_permutation[256] SOMNIA_PROGMEM = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103,
    30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,
    197, 62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174,
    20,  125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231,
    83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143,
    54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196,
    135, 130, 116, 188, 189, 141, 1,   120, 16,  22,  112, 64,  42,  218, 14,  5,   228, 121, 109,
    178, 104, 251, 127, 24,  4,   150, 254, 98,  115, 31,  224, 124, 172, 86,  170, 49,  180, 250,
    195, 183, 29,  28,  248, 202, 108, 101, 223, 82,  156, 147, 212, 152, 67,  192, 184, 114, 122,
    81,  110, 129, 214, 224, 123, 93,  25,  189, 51,  46,  212, 232, 206, 136, 159, 144, 210, 24,
    78,  188, 155, 34,  97,  84,  111, 236, 10,  61,  59,  173, 213, 238, 204, 175, 47,  85,  11,
    100, 66,  221, 153, 190, 241, 80,  220, 45,  146, 222, 255, 113, 137, 239, 101, 145, 186, 244,
    150, 124, 185, 240, 184, 229, 165, 230, 180, 193, 107, 179, 167, 191, 106, 2,   72,  249, 185,
    126, 242, 84,  181, 44,  50,  71,  128, 183};

// Knuth 32-bit multiplicative hash mixer: incorporates all 32 bits of step and seed
static inline size_t hashIndex(uint32_t i, uint32_t seed) {
    uint32_t h = (i ^ seed) * 2654435761u;
    return static_cast<size_t>((h >> 24) & 255);
}

float PatternEngine::getSample(uint32_t cell, float frac, uint32_t seed) const {
    uint32_t i0 = cell;
    uint32_t i1 = cell + 1;
    float f0 = frac;
    float f1 = f0 - 1.0f;

    size_t hashed_i0 = hashIndex(i0, seed);
    size_t hashed_i1 = hashIndex(i1, seed);

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

float PatternEngine::getSample(float t, uint32_t seed) const {
    float t_pos = t < 0.0f ? 0.0f : t;
    float floor_val = std::floor(t_pos);
    uint32_t cell = static_cast<uint32_t>(floor_val);
    float frac = t_pos - floor_val;
    return getSample(cell, frac, seed);
}

float PatternEngine::getFbmSample(uint32_t cell, float frac, uint32_t seed, int octaves) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float max_val = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        uint32_t mult = (1u << i);
        uint32_t cur_cell = cell * mult;
        float cur_frac = frac * static_cast<float>(mult);
        if (cur_frac >= 1.0f) {
            uint32_t whole = static_cast<uint32_t>(cur_frac);
            cur_cell += whole;
            cur_frac -= static_cast<float>(whole);
        }

        float sample = getSample(cur_cell, cur_frac, seed) - 0.5f;
        result += sample * amplitude;
        max_val += 0.5f * amplitude;
        amplitude *= 0.5f;
    }

    if (max_val == 0.0f) {
        return 0.5f;
    }
    return (result / max_val) * 0.5f + 0.5f;
}

float PatternEngine::getFbmSample(float t, uint32_t seed, int octaves) const {
    float t_pos = t < 0.0f ? 0.0f : t;
    float floor_val = std::floor(t_pos);
    uint32_t cell = static_cast<uint32_t>(floor_val);
    float frac = t_pos - floor_val;
    return getFbmSample(cell, frac, seed, octaves);
}

uint8_t PatternEngine::getSampleQ8(uint32_t t_q8, uint32_t seed) const {
    uint32_t i0 = t_q8 >> 8;
    int32_t f0 = static_cast<int32_t>(t_q8 & 255);
    int32_t f1 = f0 - 256;

    size_t hashed_i0 = hashIndex(i0, seed);
    size_t hashed_i1 = hashIndex(i0 + 1, seed);

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

uint8_t PatternEngine::getFbmSampleQ8(uint32_t t_q8, uint32_t seed, int octaves) const {
    int32_t result = 0;
    int32_t amp = 128;
    uint32_t freq_mult = 1;
    int32_t max_val = 0;

    for (int i = 0; i < octaves; ++i) {
        uint32_t cur_t = t_q8 * freq_mult;
        int32_t sample = static_cast<int32_t>(getSampleQ8(cur_t, seed)) - 128;
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
