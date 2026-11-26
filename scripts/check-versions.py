#!/usr/bin/env python3
"""
Verify version consistency across all project manifests and source files.
Exits with 0 if all versions match CMakeLists.txt project VERSION, 1 otherwise.
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def main() -> int:
    cmake_path = ROOT / "CMakeLists.txt"
    with open(cmake_path, "r", encoding="utf-8") as f:
        m = re.search(r"project\([^ ]+ VERSION ([0-9]+\.[0-9]+\.[0-9]+)", f.read())
        if not m:
            print("Error: Could not find VERSION in CMakeLists.txt", file=sys.stderr)
            return 1
        expected = m.group(1)

    print(f"Target version: {expected}")
    errors = 0

    # 1. bindings/python/pyproject.toml
    pyproject_path = ROOT / "bindings" / "python" / "pyproject.toml"
    with open(pyproject_path, "r", encoding="utf-8") as f:
        m = re.search(r'version\s*=\s*"([^"]+)"', f.read())
        if not m or m.group(1) != expected:
            print(f"Mismatch in {pyproject_path}: {m.group(1) if m else 'None'} != {expected}", file=sys.stderr)
            errors += 1

    # 2. bindings/python/somnia/__init__.py
    py_init = ROOT / "bindings" / "python" / "somnia" / "__init__.py"
    with open(py_init, "r", encoding="utf-8") as f:
        m = re.search(r'__version__\s*=\s*"([^"]+)"', f.read())
        if not m or m.group(1) != expected:
            print(f"Mismatch in {py_init}: {m.group(1) if m else 'None'} != {expected}", file=sys.stderr)
            errors += 1

    # 3. bindings/npm/package.json
    pkg_json = ROOT / "bindings" / "npm" / "package.json"
    with open(pkg_json, "r", encoding="utf-8") as f:
        data = json.load(f)
        pkg_ver = data.get("version", "")
        expected_prefix = ".".join(expected.split(".")[:2]) + "."
        if not (pkg_ver == expected or (pkg_ver.startswith(expected_prefix) and pkg_ver >= expected)):
            print(f"Mismatch in {pkg_json}: {pkg_ver} != {expected}", file=sys.stderr)
            errors += 1

    # 4. firmware/library.json
    lib_json = ROOT / "firmware" / "library.json"
    with open(lib_json, "r", encoding="utf-8") as f:
        data = json.load(f)
        if data.get("version") != expected:
            print(f"Mismatch in {lib_json}: {data.get('version')} != {expected}", file=sys.stderr)
            errors += 1

    # 5. firmware/library.properties
    lib_props = ROOT / "firmware" / "library.properties"
    with open(lib_props, "r", encoding="utf-8") as f:
        m = re.search(r"^version=([0-9]+\.[0-9]+\.[0-9]+)", f.read(), re.M)
        if not m or m.group(1) != expected:
            print(f"Mismatch in {lib_props}: {m.group(1) if m else 'None'} != {expected}", file=sys.stderr)
            errors += 1

    # 6. core/src/somnia_c.cpp
    c_src = ROOT / "core" / "src" / "somnia_c.cpp"
    with open(c_src, "r", encoding="utf-8") as f:
        if f'return "{expected}";' not in f.read():
            print(f"Mismatch in {c_src}: version string literal not matching {expected}", file=sys.stderr)
            errors += 1

    if errors > 0:
        print(f"FAILED: Found {errors} version mismatch(es).", file=sys.stderr)
        return 1

    print(f"SUCCESS: All {6} manifests/sources match version {expected}.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
