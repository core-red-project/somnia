#include "command.hpp"

#include <iostream>

void CommandRegistry::registerCommand(std::unique_ptr<Command> cmd) {
    if (cmd) {
        std::string n(cmd->name());
        m_commands[n] = std::move(cmd);
    }
}

Command* CommandRegistry::findCommand(std::string_view name) const {
    auto it = m_commands.find(std::string(name));
    if (it != m_commands.end()) {
        return it->second.get();
    }
    return nullptr;
}

void CommandRegistry::printHelp(const ColorTheme& c) const {
    printBanner(c);
    std::cout << c.bold << "USAGE:" << c.reset << "\n"
              << "  somnia <command> [options]\n\n"
              << c.bold << "COMMANDS:" << c.reset << "\n"
              << "  " << c.cyan << "stream" << c.reset
              << "      Live interactive terminal streaming (2D piano roll tracker)\n"
              << "  " << c.cyan << "play" << c.reset
              << "        Synthesize audio melody and play through speakers\n"
              << "  " << c.cyan << "generate" << c.reset
              << "    Emit deterministic events to stdout (text, JSON, CSV, PCM)\n"
              << "  " << c.cyan << "export" << c.reset
              << "      Export procedural composition to MIDI, WAV, JSON, or CSV file\n"
              << "  " << c.cyan << "scales" << c.reset
              << "      List all available musical scales and root note previews\n"
              << "  " << c.cyan << "bench" << c.reset
              << "       Run O(1) procedural generation latency benchmark\n\n"
              << c.bold << "OPTIONS:" << c.reset << "\n"
              << "  -s, --scale <name>     Musical scale (lydian, minor-pentatonic, dorian, etc.)\n"
              << "  -r, --root <note>      Root keynote offset (C, C#, D, D#, E, F, F#, G, G#, A, "
                 "A#, B)\n"
              << "  -b, --bpm <tempo>      Beats per minute tempo (default: 120)\n"
              << "  -n, --steps <count>    Total events to generate (default: 16)\n"
              << "  --seed <num>           Seed integer for deterministic coordinate noise\n"
              << "  --mode <mode>          Playback style: melody, arpeggio, chords\n"
              << "  --meter <meter>        Time signature: 4/4, 3/4, 7/8\n"
              << "  --timestep <float>     Coordinate step advance per event (default: 0.25)\n"
              << "  --rests                Inject procedural rests/breathing pauses\n"
              << "  -f, --format <fmt>     Output format: text, json, jsonl, csv, pcm\n"
              << "  -o, --output <file>    Export target file path (.mid, .wav, .json, .csv)\n"
              << "  --no-color             Disable ANSI terminal color output\n"
              << "  --no-delay             Disable real-time sleep pacing\n"
              << "  -v, --version          Display engine version and architecture\n"
              << "  -h, --help             Display this guide\n\n"
              << c.bold << "EXAMPLES:" << c.reset << "\n"
              << "  somnia stream --scale dorian --root D --bpm 110\n"
              << "  somnia play --scale hirajoshi --mode arpeggio --bpm 132\n"
              << "  somnia export --scale lydian --midi melody.mid --steps 64\n"
              << "  somnia generate --format pcm | aplay -r 44100 -f S16_LE\n\n";
}

void CommandRegistry::printVersion(const ColorTheme& c) const {
    std::cout << c.bold << "somnia" << c.reset << " version " << c.cyan << "0.6.0" << c.reset
              << "\n"
              << c.dim << "Core Red Project — Sxnnyside Quality Standard (MPL-2.0)" << c.reset
              << "\n"
              << "Standard: C++20 | Target Arch: Desktop POSIX & Embedded AVR\n"
              << "Engine ABI: somnia-abi/1 | Zero-Allocation O(1) Deterministic Core\n";
}
