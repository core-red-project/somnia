#pragma once

#include <Arduino.h>
#include <stdint.h>

class ArduinoAudioOutput {
public:
    explicit ArduinoAudioOutput(uint8_t pin) : m_pin(pin) {
    }

    void playTone(uint16_t frequency, uint16_t duration_ms) {
        m_active = true;
        m_startTime = ::millis();
        m_duration = duration_ms;
        ::tone(m_pin, frequency);
    }

    void update(uint32_t currentMillis) {
        if (m_active && (currentMillis - m_startTime >= m_duration)) {
            ::noTone(m_pin);
            m_active = false;
        }
    }

    bool isBusy() const {
        return m_active;
    }

private:
    uint8_t m_pin;
    bool m_active = false;
    uint32_t m_startTime = 0;
    uint32_t m_duration = 0;
};
