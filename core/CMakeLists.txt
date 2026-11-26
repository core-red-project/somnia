add_library(somnia_core_obj OBJECT
    src/scale_engine.cpp
    src/pattern_engine.cpp
    src/sequence_engine.cpp
    src/export_engine.cpp
    src/midi_engine.cpp
    src/wav_engine.cpp
    src/somnia_c.cpp
)

target_include_directories(somnia_core_obj PUBLIC include)
target_link_libraries(somnia_core_obj PUBLIC project_options)
set_target_properties(somnia_core_obj PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Static library target
add_library(somnia_core STATIC $<TARGET_OBJECTS:somnia_core_obj>)
target_include_directories(somnia_core PUBLIC include)
target_link_libraries(somnia_core PUBLIC project_options)

# Shared library for Python ctypes / external FFI bindings
add_library(somnia_shared SHARED $<TARGET_OBJECTS:somnia_core_obj>)
set_target_properties(somnia_shared PROPERTIES 
    OUTPUT_NAME "somnia_core"
    ARCHIVE_OUTPUT_NAME "somnia_shared"
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)
target_include_directories(somnia_shared PUBLIC include)
target_link_libraries(somnia_shared PUBLIC project_options)
