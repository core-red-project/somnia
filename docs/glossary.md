# Domain Glossary

Domain terminology and technical definitions used across the **Somnia** codebase and documentation.

## Musical & Audio Concepts

- **12-TET**: 12-Tone Equal Temperament. The tuning standard where each octave is divided into 12 semitones on a logarithmic frequency scale ratio of $\sqrt[12]{2} \approx 1.059463$.
- **Musical Scale**: A constrained subset of musical pitch intervals within an octave (e.g. Dorian, Lydian, Hirajoshi).
- **Scale Mode**: Interval configuration and permutation of scale degrees determining tonal color and emotional tension.
- **PPQN**: Pulses Per Quarter Note. MIDI clock resolution unit; Somnia MIDI exports encode tracks at 480 PPQN.
- **Note Event**: Discrete structural unit of music generation containing pitch, MIDI note value, duration in milliseconds, frequency, velocity, and rest state.
- **Quantization**: Constraining continuous raw noise values into discrete scalar musical intervals and rhythm increments.

## Algorithmic & Embedded Constraints

- **Value Noise**: Deterministic 1D lattice gradient function evaluating smooth continuous non-periodic values between integer coordinates.
- **FBM**: Fractal Brownian Motion. Summation of successive octaves of procedural value noise with increasing frequencies and decreasing amplitudes.
- **O(1) Engine**: System guaranteeing constant execution time and zero heap memory allocations during real-time streaming note generation.
- **Q8.8 Fixed Point**: 16-bit integer number format using 8 integer bits and 8 fractional bits to perform real-time DSP math on 8-bit microcontrollers without an FPU.
- **LUT**: Look-Up Table. Precomputed array (e.g. `kMidiFrequencies`) stored in program memory (`PROGMEM`) to avoid runtime `pow()` calculations.
