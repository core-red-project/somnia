"""
Somnia Python Bindings
Zero-Allocation Procedural Audio Engine
"""

import ctypes
import os
import sys
from typing import NamedTuple, Optional

class NoteEvent(NamedTuple):
    frequency: int
    duration_ms: int
    label: str
    midi_note: int
    velocity: int
    is_rest: bool

class _CNoteEvent(ctypes.Structure):
    _fields_ = [
        ("frequency", ctypes.c_uint16),
        ("duration_ms", ctypes.c_uint16),
        ("label", ctypes.c_char_p),
        ("midi_note", ctypes.c_uint8),
        ("velocity", ctypes.c_uint8),
        ("is_rest", ctypes.c_bool),
    ]

class Somnia:
    SCALES = {
        "lydian": 0,
        "minor-pentatonic": 1,
        "pentatonic": 1,
        "major-pentatonic": 2,
        "dorian": 3,
        "minor": 4,
        "natural-minor": 4,
        "major": 5,
        "mixolydian": 6,
        "phrygian": 7,
        "blues": 8,
        "hirajoshi": 9,
    }

    MODES = {
        "melody": 0,
        "arpeggio": 1,
        "chords": 2,
    }

    def __init__(self, lib_path: Optional[str] = None):
        if lib_path is None:
            # Search common build paths relative to this file
            base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            candidates = [
                os.path.join(base_dir, "build", "core", "libsomnia_core.dylib"),
                os.path.join(base_dir, "build", "core", "libsomnia_core.so"),
                os.path.join(base_dir, "build", "core", "somnia_core.dll"),
            ]
            for c in candidates:
                if os.path.exists(c):
                    lib_path = c
                    break

        if lib_path and os.path.exists(lib_path):
            self._lib = ctypes.CDLL(lib_path)
            self._init_c_types()
            self._handle = self._lib.somnia_create()
        else:
            self._lib = None
            self._handle = None

    def _init_c_types(self):
        self._lib.somnia_create.restype = ctypes.c_void_p
        self._lib.somnia_destroy.argtypes = [ctypes.c_void_p]
        self._lib.somnia_version.restype = ctypes.c_char_p

        self._lib.somnia_next_event.argtypes = [
            ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint8, ctypes.c_uint8,
            ctypes.c_uint16, ctypes.c_bool, ctypes.c_uint8, ctypes.c_uint8
        ]
        self._lib.somnia_next_event.restype = _CNoteEvent

        self._lib.somnia_export_midi.argtypes = [
            ctypes.c_char_p, ctypes.c_uint32, ctypes.c_uint8, ctypes.c_uint8,
            ctypes.c_uint16, ctypes.c_size_t
        ]
        self._lib.somnia_export_midi.restype = ctypes.c_bool

        self._lib.somnia_export_wav.argtypes = [
            ctypes.c_char_p, ctypes.c_uint32, ctypes.c_uint8, ctypes.c_uint8,
            ctypes.c_uint16, ctypes.c_size_t
        ]
        self._lib.somnia_export_wav.restype = ctypes.c_bool

    def __del__(self):
        if getattr(self, "_lib", None) and getattr(self, "_handle", None):
            self._lib.somnia_destroy(self._handle)

    def next_event(self, seed: int = 12345, scale: str = "lydian", root: int = 0,
                   bpm: int = 120, allow_rests: bool = False, mode: str = "melody") -> NoteEvent:
        if not self._handle:
            raise RuntimeError("Somnia library not loaded or built.")

        scale_val = self.SCALES.get(scale.lower(), 0)
        mode_val = self.MODES.get(mode.lower(), 0)

        c_ev = self._lib.somnia_next_event(
            self._handle, seed, scale_val, root, bpm, allow_rests, mode_val, 0
        )
        label = c_ev.label.decode("utf-8") if c_ev.label else ""
        return NoteEvent(
            frequency=c_ev.frequency,
            duration_ms=c_ev.duration_ms,
            label=label,
            midi_note=c_ev.midi_note,
            velocity=c_ev.velocity,
            is_rest=c_ev.is_rest,
        )

    def export_midi(self, filepath: str, seed: int = 12345, scale: str = "lydian",
                    root: int = 0, bpm: int = 120, steps: int = 32) -> bool:
        if not self._lib:
            raise RuntimeError("Somnia library not loaded.")
        scale_val = self.SCALES.get(scale.lower(), 0)
        return self._lib.somnia_export_midi(
            filepath.encode("utf-8"), seed, scale_val, root, bpm, steps
        )

    def export_wav(self, filepath: str, seed: int = 12345, scale: str = "lydian",
                   root: int = 0, bpm: int = 120, steps: int = 32) -> bool:
        if not self._lib:
            raise RuntimeError("Somnia library not loaded.")
        scale_val = self.SCALES.get(scale.lower(), 0)
        return self._lib.somnia_export_wav(
            filepath.encode("utf-8"), seed, scale_val, root, bpm, steps
        )

    @property
    def version(self) -> str:
        if self._lib:
            return self._lib.somnia_version().decode("utf-8")
        return "0.4.0"

if __name__ == "__main__":
    print("Testing Somnia Python Bindings...")
    engine = Somnia()
    print(f"Somnia Core Version: {engine.version}")
    for i in range(8):
        note = engine.next_event(seed=42 + i, scale="hirajoshi", bpm=120)
        print(f"Step {i+1}: {note.label:4s} | MIDI: {note.midi_note} | {note.frequency} Hz | Vel: {note.velocity}")
    print("Exporting test MIDI & WAV via Python bindings...")
    engine.export_midi("test_python.mid", seed=42, scale="hirajoshi", steps=16)
    engine.export_wav("test_python.wav", seed=42, scale="hirajoshi", steps=16)
    print("Python bindings test completed successfully.")

