add_executable(somnia_tests
    core_tests.cpp
    ${CMAKE_SOURCE_DIR}/cli/src/cli_args.cpp
)

target_include_directories(somnia_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/cli/include
)

target_link_libraries(somnia_tests PRIVATE somnia_core project_options)

add_test(NAME CoreUnitTests COMMAND somnia_tests)
set_tests_properties(CoreUnitTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
if(WIN32)
    set_property(TEST CoreUnitTests APPEND PROPERTY
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:somnia_shared>;$ENV{PATH}"
    )
endif()
