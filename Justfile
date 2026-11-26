# Somnia Task Runner

set shell := ["sh", "-cu"]
set windows-shell := ["sh", "-cu"]

default:
    @just --list

install:
    cmake --preset release
    cmake --preset dev
    git config core.hooksPath .githooks

dev:
    ./build/release/cli/somnia-cli stream

build:
    @if [ ! -d "build/release" ]; then cmake --preset release; fi
    cmake --build --preset release

test:
    cmake --build --preset release
    ctest --preset release --output-on-failure

typecheck:
    cmake --preset dev
    cmake --build --preset dev

lint:
    @if [ -f "build/dev/compile_commands.json" ] && command -v clang-tidy >/dev/null 2>&1; then \
        clang-tidy -p build/dev $(git ls-files "core/src/*.cpp" "cli/src/*.cpp"); \
    elif [ -f "build/dev/compile_commands.json" ] && [ -x "/opt/homebrew/opt/llvm/bin/clang-tidy" ]; then \
        /opt/homebrew/opt/llvm/bin/clang-tidy -p build/dev $(git ls-files "core/src/*.cpp" "cli/src/*.cpp"); \
    else \
        clang-format --dry-run -Werror $(git ls-files "*.cpp" "*.hpp" "*.h" "*.cc" "*.ino"); \
    fi

format:
    clang-format -i $(git ls-files "*.cpp" "*.hpp" "*.h" "*.cc" "*.ino")
    shfmt -w scripts/ .githooks/

format-check:
    clang-format --dry-run -Werror $(git ls-files "*.cpp" "*.hpp" "*.h" "*.cc" "*.ino")
    @if command -v shfmt >/dev/null 2>&1; then shfmt -d scripts/ .githooks/; fi

lint-files:
    @if command -v actionlint >/dev/null 2>&1; then actionlint .github/workflows/*.yml; fi
    @if command -v shellcheck >/dev/null 2>&1; then shellcheck scripts/*.sh .githooks/*; fi
    @if command -v shfmt >/dev/null 2>&1; then shfmt -d scripts/ .githooks/; fi

check: format-check lint typecheck test lint-files

clean:
    rm -rf build build-debug test.mid test.wav test.json test.csv dist release-assets

# --- extras: domain-specific workflows ---

coverage-threshold := "90"

run *args="": build
    ./build/release/cli/somnia-cli generate {{args}}

play *args="": build
    ./build/release/cli/somnia-cli play {{args}}

stream *args="": build
    ./build/release/cli/somnia-cli stream {{args}}

bench: build
    ./build/release/cli/somnia-cli bench

scales *args="": build
    ./build/release/cli/somnia-cli scales {{args}}

firmware:
    arduino-cli compile --fqbn arduino:avr:nano --library core firmware/
    arduino-cli compile --fqbn arduino:avr:uno --library core firmware/

firmware-pio:
    pio run -d firmware -e esp32 -e pico

package: build
    cd build/release && cpack
