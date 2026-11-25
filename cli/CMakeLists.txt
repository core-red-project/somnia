add_executable(somnia-cli
    src/main.cpp
    src/terminal_ui.cpp
    src/cli_args.cpp
    src/command.cpp
    src/subcommands.cpp
)

target_include_directories(somnia-cli PRIVATE include)
target_link_libraries(somnia-cli PRIVATE somnia_core project_options)
