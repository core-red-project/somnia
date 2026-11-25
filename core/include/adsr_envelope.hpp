#pragma once

#ifdef ARDUINO
#include <stdint.h>
#else
#include <cstdint>
#endif

// Four-stage Envelope State Machine
enum class EnvelopeState : uint8_t { Idle = 0, Attack, Decay, Sustain, Release };

// Pure integer fixed-point (Q8.8) ADSR envelope generator
// Eliminates audible clicks, pops, and provides dynamic amplitude shaping in zero heap memory
class AdsrEnvelope {
public:
    struct Parameters {
        uint16_t attack_ms;    // Attack time: ramp up from 0 to peak
        uint16_t decay_ms;     // Decay time: drop from peak to sustain
        uint8_t sustain_level; // Sustain amplitude level [0..255] (Q8: 190 ≈ 0.75)
        uint16_t release_ms;   // Release time: fade from sustain to 0

        Parameters() : attack_ms(15), decay_ms(35), sustain_level(190), release_ms(60) {
        }
        Parameters(uint16_t a, uint16_t d, uint8_t s, uint16_t r)
            : attack_ms(a), decay_ms(d), sustain_level(s), release_ms(r) {
        }
    };

    AdsrEnvelope() : m_params() {
    }

    explicit AdsrEnvelope(const Parameters& params) : m_params(params) {
    }

    // Trigger note onset
    void noteOn(uint8_t velocity = 100) {
        m_velocity = velocity;
        m_state = EnvelopeState::Attack;
        m_elapsedMs = 0;
        m_levelQ8 = 0;
        m_active = true;
    }

    // Trigger note release
    void noteOff() {
        if (m_state != EnvelopeState::Idle) {
            m_state = EnvelopeState::Release;
            m_elapsedMs = 0;
            m_releaseStartLevel = m_levelQ8;
        }
    }

    // Advance envelope time by deltaMs (called periodically or per buffer)
    void advance(uint16_t deltaMs) {
        if (!m_active || m_state == EnvelopeState::Idle) {
            return;
        }

        m_elapsedMs += deltaMs;

        switch (m_state) {
            case EnvelopeState::Attack: {
                if (m_params.attack_ms == 0 || m_elapsedMs >= m_params.attack_ms) {
                    m_levelQ8 = 255;
                    m_state = EnvelopeState::Decay;
                    m_elapsedMs = 0;
                } else {
                    // Linear ramp: 0 -> 255
                    m_levelQ8 = static_cast<uint8_t>((static_cast<uint32_t>(m_elapsedMs) * 255) /
                                                     m_params.attack_ms);
                }
                break;
            }
            case EnvelopeState::Decay: {
                if (m_params.decay_ms == 0 || m_elapsedMs >= m_params.decay_ms) {
                    m_levelQ8 = m_params.sustain_level;
                    m_state = EnvelopeState::Sustain;
                    m_elapsedMs = 0;
                } else {
                    // Ramp down from 255 -> sustain_level
                    uint32_t delta = 255 - m_params.sustain_level;
                    uint32_t drop =
                        (static_cast<uint32_t>(m_elapsedMs) * delta) / m_params.decay_ms;
                    m_levelQ8 = static_cast<uint8_t>(255 - drop);
                }
                break;
            }
            case EnvelopeState::Sustain: {
                m_levelQ8 = m_params.sustain_level;
                break;
            }
            case EnvelopeState::Release: {
                if (m_params.release_ms == 0 || m_elapsedMs >= m_params.release_ms) {
                    m_levelQ8 = 0;
                    m_state = EnvelopeState::Idle;
                    m_active = false;
                } else {
                    // Ramp down from releaseStartLevel -> 0
                    uint32_t drop = (static_cast<uint32_t>(m_elapsedMs) * m_releaseStartLevel) /
                                    m_params.release_ms;
                    m_levelQ8 = (drop >= m_releaseStartLevel)
                                    ? 0
                                    : static_cast<uint8_t>(m_releaseStartLevel - drop);
                }
                break;
            }
            case EnvelopeState::Idle:
            default:
                m_levelQ8 = 0;
                m_active = false;
                break;
        }
    }

    // Calculates current amplitude multiplier [0..255] scaled by velocity
    [[nodiscard]] uint8_t getLevel() const {
        if (!m_active) {
            return 0;
        }
        // Multiply Q8 level with velocity [0..127] -> result [0..255]
        return static_cast<uint8_t>((static_cast<uint32_t>(m_levelQ8) * m_velocity) / 127);
    }

    [[nodiscard]] bool isActive() const {
        return m_active;
    }

    [[nodiscard]] EnvelopeState getState() const {
        return m_state;
    }

    void setParameters(const Parameters& params) {
        m_params = params;
    }

private:
    Parameters m_params;
    EnvelopeState m_state = EnvelopeState::Idle;
    uint8_t m_velocity = 100;
    uint8_t m_levelQ8 = 0;
    uint8_t m_releaseStartLevel = 0;
    uint32_t m_elapsedMs = 0;
    bool m_active = false;
};
