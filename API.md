# API

The contract of this library: what a consumer may depend on, what may change, and what is not theirs to use. The tables are checked by the build; the code carries the same markers.

## Statuses

| Status | What it promises | In the code |
|---|---|---|
| `public` | Stable. Documented, tested, covered by SemVer: a breaking change needs a major version | Exported by the entry point; doc comment with an example |
| `experimental` | Exported on purpose, but may change or disappear in any minor release. Not covered by SemVer | The language's opt-in marker (see the table below); the doc comment says so |
| `deprecated` | Still works. Stays for at least one minor release, then is removed in a major | The language's deprecation marker, always naming the replacement; a changelog entry |
| `internal` | No promise. Reachable only because the language needs it | Not exported by the entry point, and marked: `_`-prefixed, `internal`, `detail`, `pub(crate)`, `@internal` |

Anything not listed in the table below is `internal`, whatever the language lets a consumer reach.

## Symbols

| Symbol | Status | Since | Replacement | Notes |
|---|---|---|---|---|
| `somnia_create()` | public | 0.4.0 | | Allocates and initializes an opaque C engine instance |
| `somnia_destroy()` | public | 0.4.0 | | Releases resources associated with an opaque C engine instance |
| `somnia_next_event()` | public | 0.4.0 | | Generates next procedural note event in O(1) time complexity |
| `somnia_export_midi()` | public | 0.4.0 | | Serializes procedural sequence to standard MIDI format 0 |
| `somnia_export_wav()` | public | 0.4.0 | | Synthesizes and serializes sequence to 16-bit 44.1kHz WAV PCM |
| `somnia_version()` | public | 0.4.0 | | Returns library semver string |
| `ScaleEngine` | public | 0.4.0 | | C++ 12-TET scale quantization and frequency lookup table |
| `PatternEngine` | public | 0.4.0 | | C++ 1D Perlin Value Noise with FBM evaluation |
| `SequenceEngine` | public | 0.4.0 | | C++ stateful generator stream and note quantization |
| `AdsrEnvelope` | public | 0.6.0 | | C++ fixed-point integer ADSR dynamic envelope generator |
| `PolyphonicMixer` | public | 0.6.0 | | C++ zero-allocation virtual polyphonic voice mixer |
| `MidiEngine` | public | 0.4.0 | | C++ standard MIDI file serializer |
| `WavEngine` | public | 0.4.0 | | C++ 16-bit 44.1kHz PCM audio synthesizer and exporter |
| `ExportEngine` | public | 0.4.0 | | C++ JSON, JSONLines, and CSV sequence exporter |
| `AudioOutput` | public | 0.4.0 | | C++ hardware and platform audio streaming interface abstraction |
| `Somnia` (Python) | public | 0.4.0 | | High-level Python engine wrapper with ctypes FFI (`somnia-audio` on PyPI) |
| `SomniaEngine` (TypeScript) | public | 0.6.0 | | TypeScript procedural generative engine |
| `playNote()` (TypeScript) | public | 0.6.0 | | Web Audio API synthesizer playback helper |
| `ScaleType` | public | 0.4.0 | | Enumeration of supported musical modes |
| `PlayMode` | public | 0.4.0 | | Enumeration of procedural melodic play modes |
| `TimeSignature` | public | 0.4.0 | | Enumeration of rhythmic time signatures |

## Internal areas

Paths and modules that are internal, and how they are marked:
- C++ internals: `core/src/`, `cli/src/`, private class members (`m_`-prefixed).
- C ABI internals: `somnia_c_note_event_t` internal fields.
- Python internals: `bindings/python/somnia/_CNoteEvent` (`_`-prefixed).
- TypeScript internals: `@internal` annotated constants in `bindings/npm/src/index.ts` (`MIDI_FREQUENCIES`, `NOTE_NAMES`, `SCALE_INTERVALS`).

## Compatibility

Versions follow SemVer. A deprecation is announced in the release that introduces it and removed no earlier than the next major release. An experimental symbol that is not promoted in two minor releases is removed or promoted.
