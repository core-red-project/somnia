# Embedded Firmware & Arduino Library

## Purpose
Microcontroller firmware and Arduino/PlatformIO library packaging for Somnia.

## Tooling
- Build tools: Arduino CLI (`arduino-cli`), PlatformIO (`pio`)
- C++ dialect: C++17 / C++20 with AVR cross-compilation support
- HAL: Dual audio backends (`arduino_audio_output.hpp` PWM Buzzer, `i2s_audio_output.hpp` I2S DAC)
- MIDI: Hardware UART (31250 baud) + TinyUSB USB-MIDI (`somnia_usb_midi.hpp`)
- Storage: Non-volatile persistent configuration (`somnia_storage.hpp`) with CRC8

## Conventions
- Strict $O(1)$ memory: zero dynamic allocations in note generation and audio loop.
- No floating-point math in AVR core execution path.
- Non-blocking loop architecture.
