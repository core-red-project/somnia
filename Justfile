# Somnia Task Runner

set shell := ["/bin/sh", "-c"]

default:
    @just --list

install:
    cmake --preset release
    cmake --preset dev
    git config core.hooksPath .githooks

dev:
    ./build/release/cli/somnia-cli stream

build:
    cmake --build --preset release

test:
    cmake --build --preset release
    ctest --preset release

typecheck:
    cmake --preset dev
    cmake --build --preset dev

lint:
    @if command -v clang-tidy >/dev/null 2>&1; then \
        clang-tidy -p build/dev core/src/*.cpp cli/src/*.cpp; \
    elif [ -x "/opt/homebrew/opt/llvm/bin/clang-tidy" ]; then \
        /opt/homebrew/opt/llvm/bin/clang-tidy -p build/dev core/src/*.cpp cli/src/*.cpp; \
    else \
        clang-format --dry-run -Werror $(find core/ cli/ tests/ firmware/ -name '*.cpp' -o -name '*.hpp' -o -name '*.ino'); \
    fi

format:
    clang-format -i $(find core/ cli/ tests/ firmware/ -name '*.cpp' -o -name '*.hpp' -o -name '*.ino')
    shfmt -w scripts/ .githooks/

format-check:
    clang-format --dry-run -Werror $(find core/ cli/ tests/ firmware/ -name '*.cpp' -o -name '*.hpp' -o -name '*.ino')
    shfmt -d scripts/ .githooks/

lint-files:
    actionlint .github/workflows/*.yml
    shellcheck scripts/*.sh .githooks/*
    shfmt -d scripts/ .githooks/

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
