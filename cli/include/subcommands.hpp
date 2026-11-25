#pragma once

#include <memory>

#include "command.hpp"

std::unique_ptr<Command> createStreamCommand();
std::unique_ptr<Command> createPlayCommand();
std::unique_ptr<Command> createGenerateCommand();
std::unique_ptr<Command> createExportCommand();
std::unique_ptr<Command> createBenchCommand();
std::unique_ptr<Command> createScalesCommand();
