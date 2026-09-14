cmake_minimum_required(VERSION 3.15)
project(Somnia VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_SANITIZERS)
    if(NOT MSVC)
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
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
set(CPACK_PACKAGE_VENDOR "Core Red Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Zero-Allocation Procedural Audio Generator for Desktop & AVR")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
