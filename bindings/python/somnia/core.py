"""Somnia Python Bindings Core.

Provides high-level Python bindings to the native C++ Somnia procedural
audio and melody generation engine using ctypes foreign function interface.
"""

import ctypes
import os
import sys
from typing import NamedTuple, Optional


class NoteEvent(NamedTuple):
    """Discrete procedural musical event yielded by the Somnia engine.

    Attributes:
        frequency: Fundamental pitch frequency in Hertz (Hz).
        duration_ms: Duration of the event in milliseconds.
        label: Western musical pitch name string (e.g. 'C4', 'A#5', 'REST').
        midi_note: Standard MIDI note number (0..127).
        velocity: Key-strike dynamic velocity (0..127).
        is_rest: True if this event represents a musical rest (silence).
    """

    frequency: int
    duration_ms: int
    label: str
    midi_note: int
    velocity: int
    is_rest: bool


class _CNoteEvent(ctypes.Structure):
    """Internal C-ABI layout matching somnia_c_note_event_t."""

    _fields_ = [
        ("frequency", ctypes.c_uint16),
        ("duration_ms", ctypes.c_uint16),
        ("label", ctypes.c_char_p),
        ("midi_note", ctypes.c_uint8),
        ("velocity", ctypes.c_uint8),
        ("is_rest", ctypes.c_bool),
    ]


class Somnia:
    """Stateful procedural audio engine and sequence generator.

    Wraps the native Somnia shared library through C foreign function interfaces,
    allowing on-the-fly algorithmic composition and DAW file exports.

    Examples:
        >>> engine = Somnia()
        >>> note = engine.next_event(scale="dorian", bpm=120)
        >>> print(note.label, note.frequency)
    """

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

    ROOTS = {
        "c": 0,
        "c#": 1,
        "db": 1,
        "d": 2,
        "d#": 3,
        "eb": 3,
        "e": 4,
        "f": 5,
        "f#": 6,
        "gb": 6,
        "g": 7,
        "g#": 8,
        "ab": 8,
        "a": 9,
        "a#": 10,
        "bb": 10,
        "b": 11,
    }

    def __init__(self, lib_path: Optional[str] = None) -> None:
        """Initializes the Somnia engine by resolving and loading the native library.

        Args:
            lib_path: Optional absolute or relative file path to libsomnia_core.
                If None, searches package directory, SOMNIA_LIB_PATH, system paths,
                and local build trees.

        Raises:
            RuntimeError: If libsomnia_core native library cannot be located or loaded.
        """
        pkg_dir = os.path.dirname(os.path.abspath(__file__))

        if lib_path is None:
            lib_path = os.environ.get("SOMNIA_LIB_PATH")

        if lib_path is None:
            # Check package bundled libraries (wheels)
            bundled = [
                os.path.join(pkg_dir, "libsomnia_core.dylib"),
                os.path.join(pkg_dir, "libsomnia_core.so"),
                os.path.join(pkg_dir, "somnia_core.dll"),
            ]
            for b in bundled:
                if os.path.exists(b):
                    lib_path = b
                    break

        if lib_path is None:
            import ctypes.util as _ctypes_util

            system_lib = _ctypes_util.find_library("somnia_core")
            if system_lib:
                lib_path = system_lib

        if lib_path is None:
            base_dir = os.path.dirname(os.path.dirname(os.path.dirname(pkg_dir)))
            candidates = [
                os.path.join(base_dir, "build", "release", "core", "libsomnia_core.dylib"),
                os.path.join(base_dir, "build", "release", "core", "libsomnia_core.so"),
                os.path.join(base_dir, "build", "release", "core", "somnia_core.dll"),
                os.path.join(base_dir, "build", "release", "core", "Release", "somnia_core.dll"),
                os.path.join(base_dir, "build", "dev", "core", "libsomnia_core.dylib"),
                os.path.join(base_dir, "build", "dev", "core", "libsomnia_core.so"),
                os.path.join(base_dir, "build", "dev", "core", "somnia_core.dll"),
                os.path.join(base_dir, "build", "dev", "core", "Debug", "somnia_core.dll"),
                os.path.join(base_dir, "build", "core", "libsomnia_core.dylib"),
                os.path.join(base_dir, "build", "core", "libsomnia_core.so"),
                os.path.join(base_dir, "build", "core", "somnia_core.dll"),
                os.path.join(base_dir, "dist", "lib", "libsomnia_core.dylib"),
                os.path.join(base_dir, "dist", "lib", "libsomnia_core.so"),
                os.path.join(base_dir, "dist", "lib", "somnia_core.dll"),
            ]
            for c in candidates:
                if os.path.exists(c):
                    lib_path = c
                    break

        if not lib_path or not os.path.exists(lib_path):
            raise RuntimeError(
                "Could not locate or load 'libsomnia_core' native library. "
                "Please set SOMNIA_LIB_PATH or compile with 'cmake --build --preset release'."
            )

        self._lib = ctypes.CDLL(lib_path)
        self._init_c_types()
        self._handle = self._lib.somnia_create()
        if not self._handle:
            raise RuntimeError("Failed to allocate native Somnia engine context.")

    def _init_c_types(self) -> None:
        """Configures ctypes argument and return types for the native C API."""
        self._lib.somnia_create.restype = ctypes.c_void_p
        self._lib.somnia_destroy.argtypes = [ctypes.c_void_p]
        self._lib.somnia_version.restype = ctypes.c_char_p

        self._lib.somnia_next_event.argtypes = [
            ctypes.c_void_p,
            ctypes.c_uint32,
            ctypes.c_uint8,
            ctypes.c_int8,
            ctypes.c_uint16,
            ctypes.c_bool,
            ctypes.c_uint8,
            ctypes.c_uint8,
        ]
        self._lib.somnia_next_event.restype = _CNoteEvent

        self._lib.somnia_export_midi.argtypes = [
            ctypes.c_char_p,
            ctypes.c_uint32,
            ctypes.c_uint8,
            ctypes.c_int8,
            ctypes.c_uint16,
            ctypes.c_size_t,
        ]
        self._lib.somnia_export_midi.restype = ctypes.c_bool

        self._lib.somnia_export_wav.argtypes = [
            ctypes.c_char_p,
            ctypes.c_uint32,
            ctypes.c_uint8,
            ctypes.c_int8,
            ctypes.c_uint16,
            ctypes.c_size_t,
        ]
        self._lib.somnia_export_wav.restype = ctypes.c_bool

    def __del__(self) -> None:
        """Releases the underlying native C engine instance."""
        if getattr(self, "_lib", None) and getattr(self, "_handle", None):
            self._lib.somnia_destroy(self._handle)
            self._handle = None

    def next_event(
        self,
        seed: int = 12345,
        scale: str = "lydian",
        root: int = 0,
        bpm: int = 120,
        has_rests: bool = False,
        mode: str = "melody",
    ) -> NoteEvent:
        """Evaluates and returns the next procedural note event in O(1) time.

        Args:
            seed: Deterministic integer seed anchor for pseudo-random noise.
            scale: Musical mode ('lydian', 'minor-pentatonic', 'dorian', etc.).
            root: Chromatic pitch transposition in semitones (-12..+12).
            bpm: Playback tempo in Beats Per Minute (20..300).
            has_rests: If True, stochastically inserts musical pauses.
            mode: Melodic movement mode ('melody', 'arpeggio', 'chords').

        Returns:
            A NoteEvent namedtuple with frequency, duration, label, and MIDI note.

        Raises:
            ValueError: If an unknown scale or mode is provided, or bpm is non-positive.
            RuntimeError: If the native engine handle is invalid.
        """
        if not self._handle:
            raise RuntimeError("Somnia engine context is uninitialized or destroyed.")

        scale_key = scale.lower()
        if scale_key not in self.SCALES:
            raise ValueError(
                f"Unknown scale '{scale}'. Valid scales: {', '.join(sorted(self.SCALES.keys()))}"
            )
        scale_val = self.SCALES[scale_key]

        mode_key = mode.lower()
        if mode_key not in self.MODES:
            raise ValueError(
                f"Unknown mode '{mode}'. Valid modes: {', '.join(sorted(self.MODES.keys()))}"
            )
        mode_val = self.MODES[mode_key]

        if bpm <= 0:
            raise ValueError("BPM must be greater than 0.")

        if isinstance(root, str):
            root_key = root.lower()
            if root_key not in self.ROOTS:
                raise ValueError(
                    f"Unknown root note '{root}'. Valid notes: C, C#, D, D#, E, F, F#, G, G#, A, A#, B."
                )
            root_val = self.ROOTS[root_key]
        else:
            root_val = int(root)

        c_ev = self._lib.somnia_next_event(
            self._handle, seed, scale_val, root_val, bpm, has_rests, mode_val, 0
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

    def export_midi(
        self,
        filepath: str,
        seed: int = 12345,
        scale: str = "lydian",
        root: int = 0,
        bpm: int = 120,
        steps: int = 32,
    ) -> bool:
        """Serializes a procedural sequence to a Standard MIDI File (SMF format 0).

        Args:
            filepath: Destination file path string on the filesystem.
            seed: Deterministic PRNG seed integer.
            scale: Musical mode string name.
            root: Chromatic transposition offset in semitones (-12..+12).
            bpm: Sequence tempo in BPM.
            steps: Number of sequential notes to generate and serialize.

        Returns:
            True if the MIDI file was written successfully, False otherwise.

        Raises:
            ValueError: If scale, bpm, or steps are invalid.
            RuntimeError: If native engine library is uninitialized.
        """
        if not self._lib:
            raise RuntimeError("Somnia library not loaded.")

        scale_key = scale.lower()
        if scale_key not in self.SCALES:
            raise ValueError(
                f"Unknown scale '{scale}'. Valid scales: {', '.join(sorted(self.SCALES.keys()))}"
            )
        scale_val = self.SCALES[scale_key]

        if bpm <= 0:
            raise ValueError("BPM must be greater than 0.")
        if steps <= 0:
            raise ValueError("Steps must be greater than 0.")

        return self._lib.somnia_export_midi(
            filepath.encode("utf-8"), seed, scale_val, root, bpm, steps
        )

    def export_wav(
        self,
        filepath: str,
        seed: int = 12345,
        scale: str = "lydian",
        root: int = 0,
        bpm: int = 120,
        steps: int = 32,
    ) -> bool:
        """Synthesizes and exports a procedural sequence to a 16-bit 44.1kHz WAV file.

        Args:
            filepath: Destination file path string on the filesystem.
            seed: Deterministic PRNG seed integer.
            scale: Musical mode string name.
            root: Chromatic transposition offset in semitones (-12..+12).
            bpm: Sequence tempo in BPM.
            steps: Number of sequential notes to synthesize.

        Returns:
            True if the WAV file was written successfully, False otherwise.

        Raises:
            ValueError: If scale, bpm, or steps are invalid.
            RuntimeError: If native engine library is uninitialized.
        """
        if not self._lib:
            raise RuntimeError("Somnia library not loaded.")

        scale_key = scale.lower()
        if scale_key not in self.SCALES:
            raise ValueError(
                f"Unknown scale '{scale}'. Valid scales: {', '.join(sorted(self.SCALES.keys()))}"
            )
        scale_val = self.SCALES[scale_key]

        if bpm <= 0:
            raise ValueError("BPM must be greater than 0.")
        if steps <= 0:
            raise ValueError("Steps must be greater than 0.")

        return self._lib.somnia_export_wav(
            filepath.encode("utf-8"), seed, scale_val, root, bpm, steps
        )

    @property
    def version(self) -> str:
        """Returns the SemVer version string of the loaded native engine."""
        if self._lib:
            return self._lib.somnia_version().decode("utf-8")
        return "0.6.0"
