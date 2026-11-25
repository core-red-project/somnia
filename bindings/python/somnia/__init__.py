"""
Somnia Python Bindings
Deterministic Zero-Allocation Procedural Audio Engine
"""

from .core import NoteEvent, Somnia

__version__ = "0.6.0"


def somnia_version() -> str:
    """Returns the package version string."""
    return __version__


__all__ = [
    "NoteEvent",
    "Somnia",
    "somnia_version",
    "__version__",
]
