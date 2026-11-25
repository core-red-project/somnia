# Architecture

## Overview

**Somnia** is a zero-dependency C++20 procedural audio generator designed for deterministic, concurrent execution across bare-metal 8-bit microcontrollers (AVR) and modern desktop operating systems.

## Components

| Component | Responsibility | Depends on |
|---|---|---|
| `core` (`somnia_core`) | Pure procedural note generation, 12-TET LUT tables, and exporters | Standalone (no surface dependencies) |
| `cli` (`somnia-cli`) | Desktop CLI, ANSI terminal piano-roll tracker, and audio synthesis daemon | `core` |
| `firmware` | Embedded sketches and Arduino library for AVR, ESP32, and RP2040 | `core` (via `SomniaCompat.hpp`) |
| `bindings` | Foreign function interface (FFI) ABI and Python ctypes bindings | `core` (`somnia_shared`) |
| `conformance` | Shared multi-surface conformance fixtures for scale determinism | None |
| `tests` | Automated test suite with AddressSanitizer and UndefinedBehaviorSanitizer | `core` |

## Data Flow

```
[Seed / Parameters]
       │
       ▼
[PatternEngine (1D Value Noise + FBM)]
       │
       ▼
[ScaleEngine (12-TET Quantization LUT)]
       │
       ▼
[SequenceEngine (Note Events)]
       │
   ┌───┴────────────────────────┐
   ▼                            ▼
[AudioOutput Stream]     [Exporters (MIDI / WAV / JSON)]
```

## Decisions

| Decision | Why | Alternatives Rejected |
|---|---|---|
| Zero heap allocation in core | Guarantees deterministic microsecond execution on AVR MCU with 2KB SRAM | Dynamic containers (`std::vector`, heap buffering) |
| Precalculated 12-TET LUT | Eliminates costly floating-point `pow()` calls on embedded processors | Runtime mathematical exponentiation |
| Q8.8 fixed-point arithmetic | Enables smooth noise interpolation without an FPU on 8-bit hardware | Single/double precision floating-point math |
| Non-monolithic engine layout | Clear separation between pure computational core and platform surfaces | Monolithic desktop executable |

## Constraints

- Memory space complexity: strictly $O(1)$ within the audio generation loop.
- AVR Compatibility: shared core files must build with `avr-gcc` without `<fstream>` or dynamic heap features.
- C++ Standard: C++20 across all desktop components.
