#pragma once

#include "adsr_envelope.hpp"

#ifdef ARDUINO
#include <stdint.h>
#else
#include <cstdint>
#endif

// Monophonic synthesizer voice with phase accumulator and ADSR envelope
struct SynthVoice {
    uint16_t frequency = 0;
    uint32_t phase = 0;
    AdsrEnvelope envelope;
    bool active = false;

    void trigger(uint16_t freq, uint8_t velocity) {
        frequency = freq;
        phase = 0;
        active = true;
        envelope.noteOn(velocity);
    }

    void release() {
        envelope.noteOff();
    }

    void stop() {
        active = false;
        frequency = 0;
        envelope.advance(65535); // force release to idle
    }

    // Synthesizes one 16-bit signed audio sample for this voice
    [[nodiscard]] int16_t renderSample(uint32_t sampleRate) {
        if (!active || frequency == 0) {
            return 0;
        }

        // Check if envelope has finished release phase
        if (!envelope.isActive()) {
            active = false;
            return 0;
        }

        uint8_t envLevel = envelope.getLevel(); // [0..255]
        if (envLevel == 0) {
            return 0;
        }

        constexpr int32_t baseAmp = 8000;
        int32_t scaledAmp = (baseAmp * envLevel) >> 8;

        // Fast parabolic sine approximation in integer math
        uint8_t p = static_cast<uint8_t>(phase >> 8);
        int32_t sample = 0;
        if (p < 128) {
            sample = ((static_cast<int32_t>(p) * (128 - p)) * scaledAmp) >> 12;
        } else {
            uint8_t inv = p - 128;
            sample = -(((static_cast<int32_t>(inv) * (128 - inv)) * scaledAmp) >> 12);
        }

        uint32_t phaseStep = (static_cast<uint32_t>(frequency) << 16) / sampleRate;
        phase += phaseStep;

        return static_cast<int16_t>(sample);
    }
};

// Polyphonic Virtual Voice Mixer (zero dynamic allocation, O(1) space)
// Supports up to MAX_VOICES concurrent notes (e.g. Lead Melody + Bass Drone + Chord)
template <size_t MAX_VOICES = 4>
class PolyphonicMixer {
public:
    PolyphonicMixer() = default;

    // Allocate note to an idle voice or steal oldest active voice
    void noteOn(uint16_t frequency, uint8_t velocity = 100) {
        if (frequency == 0) {
            return;
        }

        // 1. Look for free/inactive voice
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            if (!m_voices[i].active) {
                m_voices[i].trigger(frequency, velocity);
                return;
            }
        }

        // 2. Look for voice currently releasing
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].envelope.getState() == EnvelopeState::Release) {
                m_voices[i].trigger(frequency, velocity);
                return;
            }
        }

        // 3. Voice stealing (round-robin)
        m_stealIndex = (m_stealIndex + 1) % MAX_VOICES;
        m_voices[m_stealIndex].trigger(frequency, velocity);
    }

    void noteOff(uint16_t frequency) {
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].active && m_voices[i].frequency == frequency) {
                m_voices[i].release();
            }
        }
    }

    void allNotesOff() {
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            m_voices[i].stop();
        }
    }

    void setGlobalEnvelope(uint16_t attackMs,
                           uint16_t decayMs,
                           uint8_t sustainLevel,
                           uint16_t releaseMs) {
        AdsrEnvelope::Parameters params{attackMs, decayMs, sustainLevel, releaseMs};
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            m_voices[i].envelope.setParameters(params);
        }
    }

    void advanceEnvelopes(uint16_t deltaMs) {
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].active) {
                m_voices[i].envelope.advance(deltaMs);
                if (!m_voices[i].envelope.isActive()) {
                    m_voices[i].active = false;
                }
            }
        }
    }

    // Mix all active voice samples with headroom protection (saturating addition)
    [[nodiscard]] int16_t renderMixedSample(uint32_t sampleRate = 44100) {
        int32_t sum = 0;
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].active) {
                sum += m_voices[i].renderSample(sampleRate);
            }
        }

        // Soft saturation clamp [-32767..32767]
        if (sum > 32767) {
            sum = 32767;
        } else if (sum < -32767) {
            sum = -32767;
        }

        return static_cast<int16_t>(sum);
    }

    [[nodiscard]] size_t getActiveVoiceCount() const {
        size_t count = 0;
        for (size_t i = 0; i < MAX_VOICES; ++i) {
            if (m_voices[i].active) {
                count++;
            }
        }
        return count;
    }

private:
    SynthVoice m_voices[MAX_VOICES];
    size_t m_stealIndex = 0;
};
