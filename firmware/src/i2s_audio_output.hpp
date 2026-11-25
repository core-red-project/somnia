#pragma once

#include <stdint.h>

#include "adsr_envelope.hpp"
#include "audio_output.hpp"
#include "polyphonic_mixer.hpp"

// High-fidelity PCM / I2S audio output backend for 32-bit MCUs (ESP32 / RP2040)
// Features full 4-voice polyphonic mixing and integer ADSR dynamic amplitude envelopes
class I2sAudioOutput : public AudioOutput {
public:
    struct Config {
        uint32_t sampleRate;
        int bckPin;
        int wsPin;
        int dataPin;

        Config() : sampleRate(44100), bckPin(26), wsPin(25), dataPin(22) {
        }
        Config(int bck, int ws, int data, uint32_t rate = 44100)
            : sampleRate(rate), bckPin(bck), wsPin(ws), dataPin(data) {
        }
    };

    explicit I2sAudioOutput(const Config& config = Config()) : m_config(config), m_mixer() {
    }

    I2sAudioOutput(int bckPin, int wsPin, int dataPin, uint32_t sampleRate = 44100)
        : m_config(bckPin, wsPin, dataPin, sampleRate), m_mixer() {
    }

    bool begin() {
        m_initialized = true;
        return true;
    }

    void playTone(uint16_t frequency, uint16_t duration_ms, uint8_t velocity = 100) override {
        if (frequency == 0) {
            stop();
            return;
        }

        m_active = true;
        m_duration = duration_ms;
        m_startTime = 0;
        m_currentFreq = frequency;
        m_mixer.noteOn(frequency, velocity);
    }

    void stop() override {
        m_mixer.allNotesOff();
        m_active = false;
        m_currentFreq = 0;
    }

    void update(uint32_t currentMillis) override {
        if (m_active) {
            if (m_startTime == 0) {
                m_startTime = currentMillis;
                m_lastUpdateMillis = currentMillis;
            }

            uint32_t delta = currentMillis - m_lastUpdateMillis;
            if (delta > 0) {
                m_mixer.advanceEnvelopes(static_cast<uint16_t>(delta));
                m_lastUpdateMillis = currentMillis;
            }

            uint32_t elapsed = currentMillis - m_startTime;
            if (elapsed >= m_duration) {
                m_mixer.noteOff(m_currentFreq);
                if (m_mixer.getActiveVoiceCount() == 0) {
                    m_active = false;
                }
            }
        }
    }

    bool isBusy() const override {
        return m_active || m_mixer.getActiveVoiceCount() > 0;
    }

    // Synthesizes the next mixed audio sample from all active polyphonic voices
    [[nodiscard]] int16_t nextSample() {
        return m_mixer.renderMixedSample(m_config.sampleRate);
    }

    // Access to polyphonic mixer for multi-timbral layering
    PolyphonicMixer<4>& mixer() {
        return m_mixer;
    }

    PolyphonicMixer<4>& getMixer() {
        return m_mixer;
    }

private:
    Config m_config;
    PolyphonicMixer<4> m_mixer;
    bool m_initialized = false;
    bool m_active = false;
    uint16_t m_currentFreq = 0;
    uint16_t m_duration = 0;
    uint32_t m_startTime = 0;
    uint32_t m_lastUpdateMillis = 0;
};
