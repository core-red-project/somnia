#pragma once

#include "somnia_midi.hpp"

// USB-MIDI Class Compliant Driver (TinyUSB / Adafruit TinyUSB compatible)
// Enables native plug & play operation on RP2040 and ESP32-S2/S3 without third-party drivers
#if defined(USE_TINYUSB) || defined(ARDUINO_ARCH_RP2040) || defined(CONFIG_TINYUSB_ENABLED)
#include <Adafruit_TinyUSB.h>

class UsbMidiOutput : public MidiOutput {
public:
    UsbMidiOutput() : m_midi() {
    }

    void begin() override {
        m_midi.setStringDescriptor("Somnia Procedural Synthesizer");
        m_midi.begin();
    }

    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        m_midi.sendNoteOn(note & 0x7FU, velocity & 0x7FU, channel + 1);
    }

    void noteOff(uint8_t note, uint8_t channel = 0) override {
        m_midi.sendNoteOff(note & 0x7FU, 0, channel + 1);
    }

    void controlChange(uint8_t controller, uint8_t value, uint8_t channel = 0) override {
        m_midi.sendControlChange(controller & 0x7FU, value & 0x7FU, channel + 1);
    }

    void sendClockTick() override {
        // Realtime Clock Tick: Cable 0, 0xF8
        uint8_t packet[4] = {0x0F, MidiStatus::ClockTick, 0x00, 0x00};
        m_midi.writePacket(packet);
    }

    void sendStart() override {
        uint8_t packet[4] = {0x0F, MidiStatus::Start, 0x00, 0x00};
        m_midi.writePacket(packet);
    }

    void sendStop() override {
        uint8_t packet[4] = {0x0F, MidiStatus::Stop, 0x00, 0x00};
        m_midi.writePacket(packet);
    }

    void sendContinue() override {
        uint8_t packet[4] = {0x0F, MidiStatus::Continue, 0x00, 0x00};
        m_midi.writePacket(packet);
    }

    void flush() override {
        m_midi.flush();
    }

private:
    Adafruit_USBD_MIDI m_midi;
};
#endif
