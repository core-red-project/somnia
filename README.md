# Somnia

![Somnia banner](docs/assets/somnia-banner.png)

![Version](https://img.shields.io/badge/version-0.6.0-blue)
![License](https://img.shields.io/badge/License-MPL--2.0-green)
[![CI](https://github.com/core-red-project/somnia/actions/workflows/ci.yml/badge.svg)](https://github.com/core-red-project/somnia/actions/workflows/ci.yml)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20AVR%20%7C%20ESP32%20%7C%20RP2040-lightgrey)
[![Discord](https://img.shields.io/badge/Discord-join-5865F2)](https://discord.gg/h3hhNY8VTE)
[![Patreon](https://img.shields.io/badge/Patreon-SxnnysideProject-F96854)](https://patreon.com/SxnnysideProject)

<p align="center">
  <strong>Deterministic Value Noise ✦ Zero-Allocation O(1) Core ✦ Built for Constrained Hardware</strong><br>
  <em>A portable, zero-dependency C++20 procedural audio engine designed to stream infinite ambient melodies into resource-constrained microcontrollers and modern terminals.</em>
</p>

<p align="center">
  <a href="#about">About</a> ✦
  <a href="#features">Features</a> ✦
  <a href="#installation">Installation</a> ✦
  <a href="#usage">Usage</a> ✦
  <a href="#architecture">Architecture</a> ✦
  <a href="#contributing">Contributing</a>
</p>

---

## About

**Somnia** is a modular, zero-dependency C++20 procedural audio generator optimized for concurrent execution on bare-metal 8-bit microcontrollers (ATmega328P / Arduino) and high-performance desktop CLI environments.

Traditional algorithmic audio pipelines rely heavily on runtime dynamic memory allocation and heavy math operations (`pow`, floating-point exponentiation), making them incompatible with restricted embedded environments. Somnia bridges this gap by engineering a stateful, lazy-evaluated streaming pipeline that achieves constant $O(1)$ memory complexity, precalculated 12-TET lookups, and fixed-point Q8.8 arithmetic.

The system utilizes a seedable 1D Perlin Value Noise generator with Fractal Brownian Motion (FBM) mapped via bitwise operations to extract smooth, non-periodic mathematical outputs. These continuous values are then structurally quantized into 10 musical scale modes with customizable root notes, dynamic velocities, and BPM tempo quantization.

### Philosophy

> _"If a line of code requires dynamic memory to express art, the problem is the line, not the hardware."_

Part of the Core Red Project realm within the Sxnnyside Project ecosystem.

## Features

- **Zero-Allocation O(1) Engine**: Constexpr lookup boundaries, precalculated 12-TET frequency tables (LUT), fixed-point Q8.8 arithmetic, and zero heap allocations.
- **Sub-Microsecond Latency**: ~100 nanoseconds per note event (~9M+ notes/second throughput) with guaranteed deterministic execution.
- **10 Musical Scales**: Lydian, Minor Pentatonic, Major Pentatonic, Dorian, Natural Minor, Major, Mixolydian, Phrygian, Blues, and Hirajoshi with full chromatic root transposition.
- **Interactive TUI Stream**: Live audio streaming with real-time non-blocking 2D piano roll lanes and interactive hotkeys (`[Space]`, `[S]`, `[R]`, `[Q/ESC]`).
- **DAW & Audio Exporters**: Direct export to Standard MIDI Files (SMF Format 0, 480 PPQN), 16-bit 44.1kHz WAV PCM audio, JSON, and CSV.
- **Embedded AVR & Multi-MCU**: Native support for Arduino Uno/Nano (ATmega328P), ESP32, and Raspberry Pi Pico with potentiometer inputs and 31250 baud MIDI serial.
- **Stable C-API & Python Bindings**: Exported `somnia_c.h` ABI with dynamic library and high-level Python `ctypes` bindings.
- **Unix Pipe Composition**: Stream raw 16-bit PCM audio samples directly to `aplay`, `ffplay`, or system audio daemons.

## Installation

### Package Managers

#### Homebrew (macOS / Linux)

```bash
brew install sxnnyside-project/tap/somnia-cli
```

#### Arch Linux (AUR)

```bash
yay -S somnia-bin
```

#### Python (PyPI)

```bash
pip install somnia-audio     # uv add somnia-audio
```

#### Node.js / Bun (npm)

```bash
bun add @sxnnyside/somnia     # npm i @sxnnyside/somnia
```

#### Embedded Microcontrollers (PlatformIO)

Add `sxnnyside/somnia` to your `platformio.ini`:

```ini
[env:my_board]
lib_deps =
    sxnnyside/somnia
```

#### Embedded Microcontrollers (Arduino IDE)

Open the **Library Manager** (`Ctrl+Shift+I` or `Cmd+Shift+I`), search for `Somnia`, and click **Install**.

---

### Prebuilt Binaries

Standalone precompiled binaries for macOS (Apple Silicon arm64) and Linux (x86_64) are available on [GitHub Releases](https://github.com/core-red-project/somnia/releases).

---

### From Source

#### Prerequisites

- CMake (3.21 or newer)
- Clang or GCC (C++20 supporting compiler)
- just (command runner)

#### Build Commands

```bash
git clone https://github.com/core-red-project/somnia.git
cd somnia

just install
just build
```

## Usage

```bash
# Stream ambient procedural melodies with live 2D piano roll
somnia stream --scale dorian --bpm 120 --rests

# Synthesize and play live through computer speakers
somnia play --scale hirajoshi -n 16 --bpm 120

# Generate structured notes in JSON format
somnia generate -n 16 --scale blues --json

# Export to Standard MIDI (.mid) and 16-bit WAV audio (.wav)
somnia export --format midi -o ambient.mid --steps 32
somnia export --format wav -o ambient.wav --steps 32

# Run high-throughput O(1) microbenchmark
somnia bench
```

## Architecture

For a detailed breakdown, see [ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Contributing

Contributions are accepted. See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

Before contributing, read the [Code of Conduct](CODE_OF_CONDUCT.md).

## License

This project is licensed under **Mozilla Public License 2.0 (MPL-2.0)** — see the [LICENSE](LICENSE) file for details.

---

<p align="center">
  <strong>Somnia</strong> — A Core Red Project<br>
  <em>&copy; 2026 Sxnnyside Project</em>
</p>
