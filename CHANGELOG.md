# Changelog

All notable changes to **Somnia** are documented here.

This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## [0.6.0] — 2026-11-25

### Added

- Hardware Abstraction Layer (HAL) with dual Buzzer (PWM) and I2S DAC stereo backends.
- Zero-float integer ADSR dynamic envelope engine (`AdsrEnvelope`) and 4-voice virtual polyphonic mixer (`PolyphonicMixer`).
- Professional MIDI output stack: Hardware UART (31250 baud DIN-5/TRS) + USB-MIDI Class-Compliant (TinyUSB) + 24 PPQN real-time clock generator (`MidiClockGenerator`).
- Exponential Moving Average (EMA) integer digital low-pass filter (`EmaFilter`) and modal hysteresis selector (`HysteresisSelector`) for noise-free analog potentiometer control.
- Non-volatile persistent configuration in EEPROM / NVS (`SomniaConfig`, `StorageManager`) with CRC8 checksum and dual-action button interface (short press: mutate seed, long press: save preset).
- Official embedded library packaging conforming to PlatformIO Registry (`library.json`) and Arduino Library 1.5 specification (`library.properties`) with canonical reference sketches (`01_piezo_buzzer`, `02_usb_midi_player`, `03_i2s_ambient`).
- Native Non-POSIX Windows console support in CLI via Win32 Virtual Terminal Processing and `<conio.h>`.

### Changed

- Upgraded PRNG hash mixer to Knuth 32-bit multiplicative hash ($h = (i \oplus \text{seed}) \times 2654435761\text{u}$), eliminating the 1024-step loop periodicity and enabling full 32-bit seeds (melodic output diverges from 0.5.0).
- Separated `StreamState` time accumulation into integer cell index + phase fraction, guaranteeing precision and non-degrading streaming across millions of steps.
- Harmonized velocity dynamic range [40, 127] and duration evaluation between floating-point and Q8 fixed-point execution paths.
- Transitioned all core procedural music evaluation to fixed-point integer arithmetic (`StreamStateQ8`, `nextEventQ8`).
- Refactored CLI monolithic structure into decoupled Command Pattern (`CommandRegistry`, `Subcommands`, `TerminalRawMode`).
- Hardened cross-platform quality gates with strict zero-warning compiler enforcement.

---

## [0.5.0] — 2026-09-14

### Added

- Multi-microcontroller support for ESP32 and RP2040 architectures in firmware sketch.
- PlatformIO build configuration for dual-architecture cross-compilation.
- High-throughput O(1) latency microbenchmark recipe (`just bench`).
- Export engines for Standard MIDI Files (480 PPQN) and 16-bit 44.1kHz WAV PCM.
- Python ctypes foreign function interface bindings.

### Changed

- Transitioned C++ build flag standard from gnu++17 to modern C++20.
- Streamlined 12-TET lookup table with PROGMEM storage for AVR platforms.

---

[Unreleased]: https://github.com/core-red-project/somnia/compare/v0.6.0...HEAD
[0.6.0]: https://github.com/core-red-project/somnia/compare/v0.5.0...v0.6.0
[0.5.0]: https://github.com/core-red-project/somnia/releases/tag/v0.5.0
