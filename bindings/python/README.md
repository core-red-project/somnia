# somnia-audio

Deterministic zero-allocation procedural audio generator and ambient synthesizer engine.

### Install

```bash
pip install somnia-audio     # uv add somnia-audio
```

### Usage

```python
import somnia
from somnia import ScaleType, Somnia

engine = Somnia()
note = engine.next_event(scale="dorian", bpm=120)

if note.is_rest:
    print("REST")
else:
    print(f"Note: {note.label} | Freq: {note.frequency} Hz | Dur: {note.duration_ms} ms")
```

### API

See [API.md](https://github.com/core-red-project/somnia/blob/main/API.md) for complete stability guarantees.

### License

Mozilla Public License 2.0 (MPL-2.0)
