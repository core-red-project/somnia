# Somnia

## Purpose
A portable, zero-dependency C++20 procedural audio engine designed to stream infinite ambient melodies into resource-constrained microcontrollers and modern terminals.

## Topology
Polyglot workspace: an engine core in C++20 with a desktop CLI surface, embedded firmware, Python bindings, and TypeScript bindings.

| Package / Boundary | Path | Language | Responsibility |
|---|---|---|---|
| Core Engine | `core/` | C++20 / C ABI | Pure deterministic procedural audio generator |
| CLI Application | `cli/` | C++20 | Interactive terminal player, stream, and exporters |
| Embedded Firmware | `firmware/` | C++ (Arduino/PlatformIO) | Multi-MCU firmware, HAL, MIDI driver, and examples |
| Python Bindings | `bindings/python/` | Python | ctypes foreign function interface (`somnia-audio`) |
| NPM Package | `bindings/npm/` | TypeScript | Web Audio API and Node.js procedural sound generator |

## Commands
- `just install` — configure build environment and install git hooks
- `just dev` — start local development interactive stream
- `just check` — the gate: format-check, lint, typecheck, test, and file linting. Must pass before any commit/push.
- `just build` · `just test` · `just clean`

## Where things go
| What | Where |
|---|---|
| Core audio engine library | `core/` |
| Public C++ headers | `core/include/` |
| Core C++ implementations | `core/src/` |
| Desktop CLI application | `cli/` |
| Embedded MCU firmware & library | `firmware/` |
| Language bindings | `bindings/` |
| Conformance fixtures | `conformance/` |
| Test suite | `tests/` |
| Documentation & assets | `docs/` |

(See `docs/glossary.md` for domain terms.)

## Conventions
- C++20 standard (`-std=c++20`, `-Wall -Wextra -Wpedantic -Werror`).
- Zero heap allocations in `core/` note generation loop: memory space complexity must remain strictly O(1).
- AVR microcontroller compatibility for shared code using `SomniaCompat.hpp`.
- Note frequencies must use precalculated 12-TET lookup table, never runtime `std::pow`.
- Commits follow Conventional Commits.

## Do not
- Never perform dynamic memory allocations (`new`, `malloc`, `std::vector`) inside the core generation path.
- Never use floating-point math in AVR-shared headers or implementation.
- Never bypass the `just check` gate.
