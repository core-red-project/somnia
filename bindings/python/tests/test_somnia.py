import sys
import unittest
from pathlib import Path

# Add bindings/python directory to sys.path so somnia package is importable
repo_root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(repo_root))

from somnia import NoteEvent, Somnia, somnia_version


class TestSomniaBindings(unittest.TestCase):
    def test_somnia_version(self):
        v = somnia_version()
        self.assertEqual(v, "0.6.0")

    def test_somnia_instantiation(self):
        engine = Somnia()
        self.assertEqual(engine.version, "0.6.0")

    def test_somnia_next_event(self):
        engine = Somnia()
        ev = engine.next_event(seed=12345, scale="dorian", root=0, bpm=120)
        self.assertIsInstance(ev, NoteEvent)
        self.assertGreater(ev.frequency, 0)
        self.assertGreater(ev.duration_ms, 0)
        self.assertTrue(12 <= ev.midi_note <= 127)
        self.assertTrue(40 <= ev.velocity <= 127)

    def test_somnia_negative_root(self):
        engine = Somnia()
        # root -1 should map to B without crash or 255 overflow
        ev = engine.next_event(seed=12345, scale="dorian", root=-1, bpm=120)
        self.assertIsInstance(ev, NoteEvent)
        self.assertGreater(ev.frequency, 0)

    def test_somnia_invalid_scale(self):
        engine = Somnia()
        with self.assertRaises(ValueError):
            engine.next_event(scale="nonexistent_scale")

    def test_somnia_invalid_mode(self):
        engine = Somnia()
        with self.assertRaises(ValueError):
            engine.next_event(mode="invalid_mode")

    def test_somnia_invalid_bpm(self):
        engine = Somnia()
        with self.assertRaises(ValueError):
            engine.next_event(bpm=0)

    def test_somnia_invalid_steps(self):
        engine = Somnia()
        with self.assertRaises(ValueError):
            engine.export_midi("test.mid", steps=0)


if __name__ == "__main__":
    unittest.main()
