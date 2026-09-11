# Somnia Task Runner

set shell := ["/bin/sh", "-c"]

default:
    @just --list

# Bootstrap and configure build environment
install:
    cmake -B build -DCMAKE_BUILD_TYPE=Release

# Local development workflow
dev: build
    ./build/cli/somnia-cli stream

# Produce build artifacts (Release mode)
build:
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZERS=OFF
    cmake --build build

# Run test suite
test: build
    cd build && ctest --output-on-failure

# Strict typecheck / compiler correctness with sanitizers enabled
typecheck:
    cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
    cmake --build build-debug

# Static analysis and formatting validation
lint:
    find core/ cli/ tests/ firmware/ -name '*.cpp' -o -name '*.hpp' -o -name '*.ino' | xargs clang-format --dry-run --Werror

# Auto-format all source files
format:
    find core/ cli/ tests/ firmware/ -name '*.cpp' -o -name '*.hpp' -o -name '*.ino' | xargs clang-format -i

# Complete quality gate: format, lint, typecheck, test
check: format lint typecheck test

# Remove build artifacts and temporary files
clean:
    rm -rf build build-debug test.mid test.wav test.json test.csv

# Run procedural generation
run *args="": build
    ./build/cli/somnia-cli generate {{args}}

# Play procedural melody through computer speakers
play *args="": build
    ./build/cli/somnia-cli play {{args}}

# Stream procedural notes live (interactive TUI)
stream *args="": build
    ./build/cli/somnia-cli stream {{args}}

# Run high-throughput O(1) latency microbenchmark
bench: build
    ./build/cli/somnia-cli bench

# List musical scales and intervals
scales *args="": build
    ./build/cli/somnia-cli scales {{args}}

# Sync canonical embedded core to Arduino firmware
sync-firmware:
    cp core/include/PatternEngine.hpp firmware/
    cp core/include/ScaleEngine.hpp firmware/
    cp core/include/SequenceEngine.hpp firmware/
    cp core/include/SomniaCompat.hpp firmware/
    cp core/include/AudioOutput.hpp firmware/
    cp core/src/PatternEngine.cpp firmware/
    cp core/src/ScaleEngine.cpp firmware/
    cp core/src/SequenceEngine.cpp firmware/

# Cross-compile Arduino firmware for Nano and Uno
firmware: sync-firmware
    arduino-cli compile --fqbn arduino:avr:nano firmware/
    arduino-cli compile --fqbn arduino:avr:uno firmware/

# Build distribution packages with CPack
package: build
    cd build && cpack
