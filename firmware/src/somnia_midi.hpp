#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#include <stdint.h>
#else
#include <cstdint>
#endif

// Standard MIDI 1.0 Status Bytes
namespace MidiStatus {
constexpr uint8_t NoteOff = 0x80;
constexpr uint8_t NoteOn = 0x90;
constexpr uint8_t ControlChange = 0xB0;
constexpr uint8_t ProgramChange = 0xC0;
constexpr uint8_t PitchBend = 0xE0;
constexpr uint8_t ClockTick = 0xF8;
constexpr uint8_t Start = 0xFA;
constexpr uint8_t Continue = 0xFB;
constexpr uint8_t Stop = 0xFC;
} // namespace MidiStatus

// Professional MIDI Output Interface supporting both Hardware UART (DIN-5/TRS)
// and USB-MIDI Class-Compliant transports (TinyUSB / Adafruit TinyUSB)
class MidiOutput {
public:
    virtual ~MidiOutput() = default;

    virtual void begin() = 0;

    // Send MIDI Note On (channel 0..15, note 0..127, velocity 0..127)
    virtual void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) = 0;

    // Send MIDI Note Off
    virtual void noteOff(uint8_t note, uint8_t channel = 0) = 0;

    // Send MIDI Control Change
    virtual void controlChange(uint8_t controller, uint8_t value, uint8_t channel = 0) = 0;

    // Send Real-time MIDI Clock Tick (0xF8, 24 ticks per quarter note)
    virtual void sendClockTick() = 0;

    // Send MIDI Real-time Transport Commands
    virtual void sendStart() = 0;
    virtual void sendStop() = 0;
    virtual void sendContinue() = 0;

    // Flush pending USB packets / write buffers
    virtual void flush() = 0;
};

// Standard Hardware Serial MIDI Output (UART at 31250 baud for DIN-5 / 3.5mm TRS Type-A)
#ifdef ARDUINO
class SerialMidiOutput : public MidiOutput {
public:
    explicit SerialMidiOutput(HardwareSerial& port = Serial) : m_serial(port) {
    }

    void begin() override {
        m_serial.begin(31250); // Official MIDI 1.0 baud rate
    }

    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        m_serial.write(static_cast<uint8_t>(MidiStatus::NoteOn | (channel & 0x0FU)));
        m_serial.write(static_cast<uint8_t>(note & 0x7FU));
        m_serial.write(static_cast<uint8_t>(velocity & 0x7FU));
    }

    void noteOff(uint8_t note, uint8_t channel = 0) override {
        m_serial.write(static_cast<uint8_t>(MidiStatus::NoteOff | (channel & 0x0FU)));
        m_serial.write(static_cast<uint8_t>(note & 0x7FU));
        m_serial.write(static_cast<uint8_t>(0x00U));
    }

    void controlChange(uint8_t controller, uint8_t value, uint8_t channel = 0) override {
        m_serial.write(static_cast<uint8_t>(MidiStatus::ControlChange | (channel & 0x0FU)));
        m_serial.write(static_cast<uint8_t>(controller & 0x7FU));
        m_serial.write(static_cast<uint8_t>(value & 0x7FU));
    }

    void sendClockTick() override {
        m_serial.write(MidiStatus::ClockTick);
    }

    void sendStart() override {
        m_serial.write(MidiStatus::Start);
    }

    void sendStop() override {
        m_serial.write(MidiStatus::Stop);
    }

    void sendContinue() override {
        m_serial.write(MidiStatus::Continue);
    }

    void flush() override {
        m_serial.flush();
    }

private:
    HardwareSerial& m_serial;
};
#endif

// High-speed MIDI Clock Generator (24 PPQN: Pulses Per Quarter Note)
// Calculates deterministic timing ticks for tempo synchronization with external gear and DAWs
class MidiClockGenerator {
public:
    MidiClockGenerator() = default;

    void setBpm(uint16_t bpm) {
        if (bpm == 0) {
            bpm = 120;
        }
        m_bpm = bpm;
        // Interval in microseconds for 24 pulses per quarter note:
        // interval_us = (60,000,000 / (bpm * 24)) = (2,500,000 / bpm)
        m_tickIntervalUs = 2500000UL / bpm;
    }

    void start(MidiOutput& midi, uint32_t currentMicros) {
        m_running = true;
        m_lastTickMicros = currentMicros;
        m_tickCounter = 0;
        midi.sendStart();
    }

    void stop(MidiOutput& midi) {
        m_running = false;
        midi.sendStop();
    }

    // Call inside Arduino loop() to emit clock pulses on-time with zero jitter
    void update(MidiOutput& midi, uint32_t currentMicros) {
        if (!m_running) {
            return;
        }

        uint32_t elapsed = currentMicros - m_lastTickMicros;
        while (elapsed >= m_tickIntervalUs) {
            midi.sendClockTick();
            m_lastTickMicros += m_tickIntervalUs;
            m_tickCounter++;
            elapsed -= m_tickIntervalUs;
        }
    }

    [[nodiscard]] bool isRunning() const {
        return m_running;
    }

    [[nodiscard]] uint32_t getTickCount() const {
        return m_tickCounter;
    }

private:
    uint16_t m_bpm = 120;
    uint32_t m_tickIntervalUs = 20833; // 120 BPM default
    uint32_t m_lastTickMicros = 0;
    uint32_t m_tickCounter = 0;
    bool m_running = false;
};
