add_executable(somnia-cli
    src/main.cpp
)

target_link_libraries(somnia-cli PRIVATE somnia_core)
