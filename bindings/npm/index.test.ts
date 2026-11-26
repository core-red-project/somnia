import { describe, expect, it } from "bun:test";
import { ScaleType, SomniaEngine } from "./src/index";

describe("Somnia TypeScript Engine", () => {
    it("initializes SomniaEngine with default values", () => {
        const engine = new SomniaEngine(424242);
        expect(engine).toBeDefined();
        // Request note with hasRests = false to guarantee pitched note
        const ev = engine.nextEvent(ScaleType.Dorian, 0, 120, false);
        expect(ev.frequency).toBeGreaterThan(0);
        expect(ev.durationMs).toBeGreaterThan(0);
        expect(ev.label.length).toBeGreaterThan(0);
        expect(ev.midiNote).toBeGreaterThanOrEqual(0);
        expect(ev.midiNote).toBeLessThanOrEqual(127);
        expect(ev.isRest).toBe(false);
    });

    it("generates deterministic notes for seed 12345", () => {
        const engine1 = new SomniaEngine(12345);
        const engine2 = new SomniaEngine(12345);

        for (let i = 0; i < 8; ++i) {
            const ev1 = engine1.nextEvent(ScaleType.Dorian, 0, 120, false);
            const ev2 = engine2.nextEvent(ScaleType.Dorian, 0, 120, false);
            expect(ev1.midiNote).toBe(ev2.midiNote);
            expect(ev1.frequency).toBe(ev2.frequency);
            expect(ev1.durationMs).toBe(ev2.durationMs);
            expect(ev1.velocity).toBe(ev2.velocity);
        }
    });

    it("matches C++ core conformance fixtures exactly", () => {
        // Conformance Test 1: C Dorian (seed 12345, 120 bpm, 4 steps, root 0, no rests)
        const engineDorian = new SomniaEngine(12345);
        const dorianNotes: number[] = [];
        for (let i = 0; i < 4; ++i) {
            const ev = engineDorian.nextEvent(ScaleType.Dorian, 0, 120, false);
            dorianNotes.push(ev.midiNote);
        }
        expect(dorianNotes).toEqual([98, 79, 87, 60]);

        // Conformance Test 2: Hirajoshi (seed 42, 100 bpm, 4 steps, root 9, no rests)
        const engineHirajoshi = new SomniaEngine(42);
        const hirajoshiNotes: number[] = [];
        for (let i = 0; i < 4; ++i) {
            const ev = engineHirajoshi.nextEvent(ScaleType.Hirajoshi, 9, 100, false);
            hirajoshiNotes.push(ev.midiNote);
        }
        expect(hirajoshiNotes).toEqual([93, 88, 76, 95]);
    });

    it("throws RangeError on non-positive BPM", () => {
        const engine = new SomniaEngine();
        expect(() => engine.nextEvent(ScaleType.Dorian, 0, 0)).toThrow(RangeError);
        expect(() => engine.nextEvent(ScaleType.Dorian, 0, -10)).toThrow(RangeError);
    });

    it("resets internal state upon reset() call", () => {
        const engine = new SomniaEngine(999);
        const ev1 = engine.nextEvent(ScaleType.MinorPentatonic, 0, 120, false);
        engine.reset(999);
        const ev2 = engine.nextEvent(ScaleType.MinorPentatonic, 0, 120, false);
        expect(ev1.midiNote).toBe(ev2.midiNote);
        expect(ev1.frequency).toBe(ev2.frequency);
    });
});
