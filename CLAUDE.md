# Somnia Developer Guide

## Overview

**Somnia** is a zero-dependency C++20 procedural audio generation engine designed for concurrent execution across high-performance desktop environments (CLI) and resource-constrained 8-bit microcontrollers (AVR / ATmega328P).

**Topology:** Monolithic (single C++20 target produces the `somnia-cli` binary and `libsomnia_core` static library, with shared embedded firmware assets in `firmware/`).

---

## Command Surface

All project workflows are exposed via `just`:

```bash
just install       # Bootstrap CMake build directory (Release mode)
just dev           # Run local interactive stream
just build         # Compile all targets in Release mode
just test          # Run ctest test suite
just typecheck     # Strict debug build with Address and UB sanitizers
just lint          # Verify formatting style with clang-format
just format        # Auto-format all C++ and Arduino source files
just check         # Full quality gate: format, lint, typecheck, test
just clean         # Remove build artifacts and caches
```

### Domain-Specific Commands

```bash
just run [args]    # Generate notes (e.g. just run "-n 16 --scale dorian")
just play [args]   # Play procedural audio live through computer speakers
just stream [args] # Start live interactive TUI streaming generator
just bench         # Run O(1) microsecond latency microbenchmark
just scales        # List all available scales and musical intervals
just sync-firmware # Sync core files to the standalone Arduino firmware folder
just package       # Build distribution packages with CPack (.tar.gz, .sh)
```

---

## Architecture & Codebase Map

```
somnia/
├── core/             # Pure C++20 procedural melody generation core
│   ├── include/      # ScaleEngine, SequenceEngine, PatternEngine, MidiEngine, WavEngine
│   └── src/          # Zero-allocation implementations, 12-TET LUT tables
├── cli/              # Desktop CLI application with subcommands and interactive TUI
│   └── src/main.cpp  # Argument parser, ANSI tracker roll, signal handlers
├── firmware/         # Standalone Arduino sketch and Arduino Library
│   ├── firmware.ino  # Cooperative non-blocking loop with analog pots & MIDI UART
│   ├── platformio.ini# PlatformIO configuration for Nano, Uno, ESP32, Pico
│   └── library.properties # Arduino Library Manager manifest
├── tests/            # Test suite covering scales, determinism, MIDI, and WAV
│   └── core_tests.cpp# Assertions verifying bounds, normalization, and formats
├── CMakeLists.txt    # Root build definition with CPack packaging rules
└── Justfile          # Canonical command surface
```

---

## Architectural Constraints & Invariants

1. **Zero Heap Allocations in Core**:
   * `ScaleEngine`, `PatternEngine`, and `SequenceEngine` must never allocate on the heap (`new`, `malloc`, or dynamic containers like `std::vector`) inside the note generation loop.
   * State is passed via `StreamState&`. Memory space complexity must remain strictly $O(1)$.
2. **AVR Microcontroller Compatibility**:
   * Any code shared with `firmware/` must be compatible with 8-bit AVR (`avr-gcc`).
   * Use `SomniaCompat.hpp` abstractions (`std_compat::array`, `std_compat::span`, `std_compat::string_view`).
   * Large static arrays (like `kMidiFrequencies` and `m_permutation`) must use `SOMNIA_PROGMEM` and `SOMNIA_READ_*` macros to avoid consuming SRAM.
   * Do not include `<fstream>` or dynamic desktop-only headers in files shared with `firmware/`.
3. **12-TET Look-Up Table (LUT)**:
   * Note frequencies must be looked up in the precalculated table (`kMidiFrequencies`), never calculated using floating-point `std::pow` at runtime.
4. **Desktop Exporters Separation**:
   * Exporters (`MidiEngine`, `WavEngine`, `ExportEngine`) belong to desktop/CLI execution and may use standard C++ libraries (`std::vector`, `std::string`, `<fstream>`).

---

## Coding Style & Quality Standards

- **Standard**: C++20 (`-std=c++20`, `-Wall -Wextra -Wpedantic -Werror`).
- **Formatting**: Enforced via `.clang-format` (`just format` / `just lint`).
- **Sanitizers**: AddressSanitizer and UndefinedBehaviorSanitizer run during `just typecheck`.
- **Testing**: Deterministic regression tests in `tests/core_tests.cpp`.
