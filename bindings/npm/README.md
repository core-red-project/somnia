# @sxnnyside/somnia

Deterministic zero-allocation procedural audio generator and ambient synthesizer engine for TypeScript, Node.js, and the Web Audio API.

### Install

```bash
bun add @sxnnyside/somnia     # npm i @sxnnyside/somnia
```

### Usage

```ts
import { SomniaEngine, ScaleType } from "@sxnnyside/somnia";

const engine = new SomniaEngine(424242);
const note = engine.nextEvent(ScaleType.Dorian, 0, 120);

if (note.isRest) {
  console.log("REST");
} else {
  console.log(`${note.label} | ${note.frequency} Hz | ${note.durationMs} ms`);
}
```

### API

Full API reference is documented with TSDoc annotations. See [API.md](https://github.com/core-red-project/somnia/blob/main/API.md) for stability guarantees.

### Browser (Web Audio API)

```ts
import { SomniaEngine, ScaleType, playNote } from "@sxnnyside/somnia";

const ctx = new AudioContext();
const engine = new SomniaEngine(1337);

function playNext() {
  const note = engine.nextEvent(ScaleType.Lydian, 0, 90);
  playNote(ctx, note);
  setTimeout(playNext, note.durationMs);
}

document.getElementById("start-btn")?.addEventListener("click", () => {
  ctx.resume().then(playNext);
});
```

### License

Mozilla Public License 2.0 (MPL-2.0)
