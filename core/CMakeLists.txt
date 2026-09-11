add_library(somnia_core STATIC
    src/ScaleEngine.cpp
    src/PatternEngine.cpp
    src/SequenceEngine.cpp
    src/ExportEngine.cpp
    src/MidiEngine.cpp
    src/WavEngine.cpp
    src/somnia_c.cpp
)

target_include_directories(somnia_core PUBLIC include)

# Optional shared library for Python ctypes / external FFI bindings
add_library(somnia_shared SHARED
    src/ScaleEngine.cpp
    src/PatternEngine.cpp
    src/SequenceEngine.cpp
    src/ExportEngine.cpp
    src/MidiEngine.cpp
    src/WavEngine.cpp
    src/somnia_c.cpp
)

set_target_properties(somnia_shared PROPERTIES OUTPUT_NAME "somnia_core")
target_include_directories(somnia_shared PUBLIC include)
