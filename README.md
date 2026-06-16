# Somnia

![License](https://img.shields.io/github/license/core-red-project/somnia)
![CI](https://github.com/core-red-project/somnia/actions/workflows/ci.yml/badge.svg)

<p align="center">
  <strong>Deterministic Value Noise ✦ Zero-Allocation O(1) Core ✦ Built for Constrained AVR Hardware</strong><br>
  <em>A portable, standalone C++20 procedural melody generation engine designed to stream infinite oníric landscapes into resource-constrained systems without heap allocations.</em>
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

**Somnia** is a modular, zero-dependency C++20 procedural audio generator optimized for concurrent execution on both high-performance desktop CLI environments and bare-metal 8-bit microcontrollers.

Traditional audio generation pipelines rely heavily on runtime dynamic memory allocation, making them incompatible with restricted embedded environments like the ATmega328P. Somnia bridges this gap by engineering a stateful, lazy-evaluated streaming pipeline that achieves constant O(1) memory space complexity.

The system utilizes a seedable 1D Perlin Value Noise generator mapped via bitwise operations to extract smooth, non-periodic mathematical outputs. These continuous values are then structurally quantized into an isolated Lydian Mode or Minor Pentatonic scale layout, outputting real-time temporal audio telemetry events.

### Philosophy

> *"If a line of code requires dynamic memory to express art, the problem is the line, not the hardware."*

This is a CoreRed project, part of the Sxnnyside Project's experimental branch.

## Features

- **Deterministic Noise Engine**: Stateful 1D Perlin Value Noise mapped deterministically via dynamic seed injections.
- **Zero-Allocation Core**: Constexpr lookup boundaries and static span mappings avoiding dynamic heap allocations.
- **Stateful O(1) Streaming**: Sequential audio telemetry streaming iterator designed for 8-bit AVR microcontrollers.

## Installation

### Prerequisites

- CMake (3.15 or newer)
- Clang/GCC (C++20 supporting compiler)

### From Source

```bash
git clone https://github.com/core-red-project/somnia.git
cd somnia

make configure
make build
```

## Usage

Run the CLI client:
```bash
make run
```
Output telemetry format:
```text
[LABEL] ([FREQUENCY]Hz) -> [DURATION]ms
```

## Architecture

```
somnia/
├── core/         # Procedural generation engine (headers and implementations)
├── cli/          # Desktop console interface runner
└── firmware/     # Cooperative scheduling firmware target for ATmega328P
```

## Contributing

Contributions are accepted. See CONTRIBUTING.md for guidelines.

Before contributing, read the Code of Conduct.

## License

This project is licensed under the MIT License — see the LICENSE file for details.

---

<p align="center">
  <strong>Somnia</strong> — This is a CoreRed project, part of the Sxnnyside Project's experimental branch.<br>
  <em>&copy; 2026 Sxnnyside Project</em>
</p>
