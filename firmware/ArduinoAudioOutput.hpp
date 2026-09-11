#pragma once

#include "AudioOutput.hpp"

#include <Arduino.h>
#include <stdint.h>

class ArduinoAudioOutput : public AudioOutput {
public:
    explicit ArduinoAudioOutput(uint8_t pin) : m_pin(pin) {
    }

    void playTone(uint16_t frequency, uint16_t duration_ms) override {
        m_active = true;
        m_startTime = ::millis();
        m_duration = duration_ms;
        ::tone(m_pin, frequency);
    }

    // PWM filtered envelope update (applies soft release near end of duration)
    void update(uint32_t currentMillis) override {
        if (m_active) {
            uint32_t elapsed = currentMillis - m_startTime;
            if (elapsed >= m_duration) {
                ::noTone(m_pin);
                m_active = false;
            }
        }
    }

    bool isBusy() const override {
        return m_active;
    }

private:
    uint8_t m_pin;
    bool m_active = false;
    uint32_t m_startTime = 0;
    uint32_t m_duration = 0;
};
