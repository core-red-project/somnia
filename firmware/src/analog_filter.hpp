#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
#endif

// Exponential Moving Average (EMA) Digital Low-Pass Filter
// Pure integer arithmetic with power-of-two alpha factor (alpha = 1 / (2^Shift))
// O(1) execution time, zero float, zero heap allocations.
template <uint8_t Shift = 3>
class EmaFilter {
public:
    constexpr explicit EmaFilter(uint16_t initialValue = 0) noexcept
        : m_value(static_cast<int32_t>(initialValue) << 8), m_initialized(initialValue != 0) {
    }

    // Process new raw ADC sample and return smoothed integer reading
    uint16_t update(uint16_t rawValue) noexcept {
        int32_t inputScaled = static_cast<int32_t>(rawValue) << 8;
        if (!m_initialized) {
            m_value = inputScaled;
            m_initialized = true;
            return rawValue;
        }

        // y[n] = y[n-1] + (x[n] - y[n-1]) >> Shift
        int32_t diff = inputScaled - m_value;
        m_value += (diff >> Shift);

        // Return rounded value (+128 for 0.5 round in Q8)
        int32_t rounded = (m_value + 128) >> 8;
        if (rounded < 0) {
            return 0;
        }
        return static_cast<uint16_t>(rounded);
    }

    void reset(uint16_t initialValue) noexcept {
        m_value = static_cast<int32_t>(initialValue) << 8;
        m_initialized = true;
    }

    [[nodiscard]] uint16_t getValue() const noexcept {
        int32_t rounded = (m_value + 128) >> 8;
        if (rounded < 0) {
            return 0;
        }
        return static_cast<uint16_t>(rounded);
    }

    [[nodiscard]] bool isInitialized() const noexcept {
        return m_initialized;
    }

private:
    int32_t m_value = 0;
    bool m_initialized = false;
};

// Hysteresis Selector for Discrete Modal Controls (Scales, Modes, Waveforms)
// Eliminates rapid toggling/chattering near boundary thresholds due to ADC noise.
template <uint8_t NumStates, uint16_t AdcMax = 1023, uint16_t Hysteresis = 12>
class HysteresisSelector {
    static_assert(NumStates > 1, "NumStates must be greater than 1");

public:
    constexpr explicit HysteresisSelector(uint8_t initialState = 0) noexcept
        : m_currentState(initialState < NumStates ? initialState : 0) {
    }

    // Update state based on raw ADC reading with deadband hysteresis protection
    uint8_t update(uint16_t rawValue) noexcept {
        if (rawValue > AdcMax) {
            rawValue = AdcMax;
        }

        constexpr uint32_t bucketSize = (static_cast<uint32_t>(AdcMax) + 1U) / NumStates;

        if (!m_initialized) {
            uint32_t rawState = static_cast<uint32_t>(rawValue) / bucketSize;
            if (rawState >= NumStates) {
                rawState = NumStates - 1U;
            }
            m_currentState = static_cast<uint8_t>(rawState);
            m_initialized = true;
            return m_currentState;
        }

        // Check transition upwards: must exceed threshold + hysteresis
        while (m_currentState < (NumStates - 1U)) {
            uint32_t upperThreshold = static_cast<uint32_t>(m_currentState + 1U) * bucketSize;
            if (static_cast<uint32_t>(rawValue) >= (upperThreshold + Hysteresis)) {
                m_currentState++;
            } else {
                break;
            }
        }

        // Check transition downwards: must fall below threshold - hysteresis
        while (m_currentState > 0) {
            uint32_t lowerThreshold = static_cast<uint32_t>(m_currentState) * bucketSize;
            if (lowerThreshold >= Hysteresis &&
                static_cast<uint32_t>(rawValue) <= (lowerThreshold - Hysteresis)) {
                m_currentState--;
            } else {
                break;
            }
        }

        return m_currentState;
    }

    [[nodiscard]] uint8_t getState() const noexcept {
        return m_currentState;
    }

    void setState(uint8_t state) noexcept {
        if (state < NumStates) {
            m_currentState = state;
            m_initialized = true;
        }
    }

    [[nodiscard]] bool isInitialized() const noexcept {
        return m_initialized;
    }

private:
    uint8_t m_currentState = 0;
    bool m_initialized = false;
};
