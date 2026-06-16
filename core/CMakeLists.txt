add_library(somnia_core STATIC
    src/ScaleEngine.cpp
    src/PatternEngine.cpp
    src/SequenceEngine.cpp
    src/ExportEngine.cpp
)

target_include_directories(somnia_core PUBLIC include)
