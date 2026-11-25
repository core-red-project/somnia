cmake_minimum_required(VERSION 3.21)
project(Somnia VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(project_options INTERFACE)
if(MSVC)
    target_compile_options(project_options INTERFACE /W4 /WX)
    target_compile_definitions(project_options INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(project_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
    if(ENABLE_SANITIZERS)
        target_compile_options(project_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(project_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

add_subdirectory(core)
add_subdirectory(cli)

enable_testing()
add_subdirectory(tests)

# Installation targets for product distribution
install(TARGETS somnia-cli RUNTIME DESTINATION bin)
install(TARGETS somnia_core ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY core/include/ DESTINATION include/somnia FILES_MATCHING PATTERN "*.hpp")

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "somnia")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Core Red Project by Sxnnyside Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Zero-Allocation Procedural Audio Generator for Desktop & AVR")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
