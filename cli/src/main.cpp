#include <iostream>

#include "cli_args.hpp"
#include "command.hpp"
#include "subcommands.hpp"
#include "terminal_ui.hpp"

int main(int argc, char* argv[]) noexcept {
    try {
        CliConfig cfg = parseCommandLine(argc, argv);
        ColorTheme color = ColorTheme::create(cfg.noColor);

        if (cfg.parseError) {
            std::cerr << color.red << "Error: " << cfg.errorMessage << color.reset << "\n\n";
            return 1;
        }

        CommandRegistry registry = CommandRegistry::createDefault();

        if (cfg.command == "help") {
            registry.printHelp(color);
            return 0;
        }

        if (cfg.command == "version") {
            registry.printVersion(color);
            return 0;
        }

        Command* cmd = registry.findCommand(cfg.command);
        if (cmd != nullptr) {
            return cmd->execute(cfg, color);
        }

        std::cerr << color.red << "Unknown command: " << cfg.command << color.reset << "\n\n";
        registry.printHelp(color);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal unknown error\n";
        return 1;
    }
}
