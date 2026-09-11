# Changelog

All notable changes to **Somnia** are documented here.

This project follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added

### Fixed

### Changed

---

## [0.4.0] — 2026-09-11

### Added

- **Modern CLI**: Complete subcommands suite (`play`, `stream`, `generate`, `export`, `scales`, `bench`, `version`, `help`) with flags `-v`, `-q`, `--json`, `-o`, `--format`.
- **Native Speaker Audio Playback**: Subcommand `somnia play` synthesizes and streams real-time generative audio through system speakers with synced ANSI spectrum visualizer.
- **Direct DAW & Audio Exporters**:
  - `MidiEngine`: Export Standard MIDI Files (.mid Format 0) with variable-length quantity encoding and tempo meta events.
  - `WavEngine`: Synthesizes 16-bit 44.1 kHz PCM WAV audio with dual-harmonic timbre and anti-click ADSR envelopes.
  - `ExportEngine`: Structured JSON, JSONLines, and CSV output formatters for Unix pipe composition.
- **Musical Scale Engine**: Expanded from 2 to 10 scales (*Lydian*, *Minor Pentatonic*, *Major Pentatonic*, *Dorian*, *Natural Minor*, *Major*, *Mixolydian*, *Phrygian*, *Blues*, *Hirajoshi*).
- **Root Note Transposition**: Full 12-semitone chromatic root selection (`--root C, D#, F, etc.`).
- **AVR Flash Memory Optimization**: `SOMNIA_PROGMEM` macro storing 12-TET LUT tables and permutation arrays in Flash ROM, saving over 512 bytes of SRAM on 8-bit AVR microcontrollers.
- **Fixed-Point Q8.8 Math**: Added `getSampleQ8` and `getFbmSampleQ8` for non-FPU embedded processors.
- **Hardware Firmware Enhancements**:
  - Support for analog potentiometer inputs on pins A0 (Tempo), A1 (Scale), and A2 (Seed).
  - Hardware serial MIDI transmission at 31250 baud.
  - `platformio.ini` supporting Arduino Uno, Nano, ESP32, and Raspberry Pi Pico.
  - `library.properties` for official Arduino Library Manager compatibility.
- **C-API & Python Bindings**: Exported stable ABI `somnia_c.h` and Python `ctypes` wrapper `bindings/python/somnia.py`.

### Changed

- Refactored `ScaleEngine` to use $O(1)$ precalculated 12-TET frequencies Look-Up Table (LUT), eliminating runtime `std::pow` float exponentiation.
- Replaced Makefile with Justfile as unified command runner.
- Upgraded project standard to C++20 across all targets.

### Removed

- Removed orphaned boilerplate `main.cpp` in repository root.

---

## [0.1.0] — 2026-06-16

### Added

- Zero-allocation C++20 ScaleEngine and PatternEngine cores.
- Stateful O(1) SequenceEngine streaming iteration API.
- CMake multi-module build configurations.
- Asynchronous non-blocking Arduino firmware with F() macro SRAM shielding.
- Clang++ and ctest unit testing harness.

---

[Unreleased]: https://github.com/core-red-project/somnia/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/core-red-project/somnia/compare/v0.1.0...v0.4.0
[0.1.0]: https://github.com/core-red-project/somnia/releases/tag/v0.1.0
