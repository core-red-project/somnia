#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "cli_args.hpp"
#include "terminal_ui.hpp"

class Command {
public:
    virtual ~Command() = default;
    [[nodiscard]] virtual std::string_view name() const = 0;
    [[nodiscard]] virtual std::string_view description() const = 0;
    virtual int execute(const CliConfig& cfg, const ColorTheme& theme) = 0;
};

class CommandRegistry {
public:
    void registerCommand(std::unique_ptr<Command> cmd);
    [[nodiscard]] Command* findCommand(std::string_view name) const;
    void printHelp(const ColorTheme& theme) const;
    void printVersion(const ColorTheme& theme) const;

    static CommandRegistry createDefault();

private:
    std::unordered_map<std::string, std::unique_ptr<Command>> m_commands;
};
