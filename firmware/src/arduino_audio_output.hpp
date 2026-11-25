#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "audio_output.hpp"

// Lightweight, zero-allocation PWM buzzer backend for AVR, ESP32, and RP2040
class BuzzerAudioOutput : public AudioOutput {
public:
    explicit BuzzerAudioOutput(uint8_t pin) : m_pin(pin) {
    }

    void playTone(uint16_t frequency, uint16_t duration_ms, uint8_t velocity = 100) override {
        (void)velocity;
        if (frequency == 0) {
            stop();
            return;
        }

        m_active = true;
        m_startTime = ::millis();
        m_duration = duration_ms;
        ::tone(m_pin, frequency);
    }

    void stop() override {
        if (m_active) {
            ::noTone(m_pin);
            m_active = false;
        }
    }

    void update(uint32_t currentMillis) override {
        if (m_active) {
            uint32_t elapsed = currentMillis - m_startTime;
            if (elapsed >= m_duration) {
                stop();
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

// Retained alias for backward compatibility
using ArduinoAudioOutput = BuzzerAudioOutput;
