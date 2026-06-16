add_executable(somnia_tests
    core_tests.cpp
)

target_link_libraries(somnia_tests PRIVATE somnia_core)

add_test(NAME CoreUnitTests COMMAND somnia_tests)
