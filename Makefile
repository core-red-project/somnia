.PHONY: all configure build run format test clean

all: build

configure:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build build

run: build
	./build/cli/somnia-cli

format:
	find core/ cli/ tests/ firmware/ -name '*.cpp' -o -name '*.hpp' -o -name '*.ino' | xargs clang-format -i

test: build
	cd build && ctest --output-on-failure

clean:
	rm -rf build
