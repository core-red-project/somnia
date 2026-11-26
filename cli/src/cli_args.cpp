#include "cli_args.hpp"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>

#include "scale_engine.hpp"

namespace {
uint16_t parseStrictBpm(const std::string& str) {
    char* end = nullptr;
    errno = 0;
    unsigned long val = std::strtoul(str.c_str(), &end, 10);
    if (errno != 0 || end == str.c_str() || *end != '\0' || val == 0 || val > 999) {
        throw std::invalid_argument("Invalid BPM value '" + str +
                                    "'. Expected integer between 1 and 999.");
    }
    return static_cast<uint16_t>(val);
}

size_t parseStrictSteps(const std::string& str) {
    char* end = nullptr;
    errno = 0;
    unsigned long val = std::strtoul(str.c_str(), &end, 10);
    if (errno != 0 || end == str.c_str() || *end != '\0' || val == 0) {
        throw std::invalid_argument("Invalid steps value '" + str +
                                    "'. Expected positive integer.");
    }
    return static_cast<size_t>(val);
}

uint32_t parseStrictSeed(const std::string& str) {
    char* end = nullptr;
    errno = 0;
    unsigned long val = std::strtoul(str.c_str(), &end, 10);
    if (errno != 0 || end == str.c_str() || *end != '\0') {
        throw std::invalid_argument("Invalid seed value '" + str +
                                    "'. Expected 32-bit unsigned integer.");
    }
    return static_cast<uint32_t>(val);
}

float parseStrictTimeStep(const std::string& str) {
    char* end = nullptr;
    errno = 0;
    float val = std::strtof(str.c_str(), &end);
    if (errno != 0 || end == str.c_str() || *end != '\0' || val <= 0.0f) {
        throw std::invalid_argument("Invalid timestep value '" + str +
                                    "'. Expected positive float.");
    }
    return val;
}
} // namespace

CliConfig parseCommandLine(int argc, char* argv[]) {
    CliConfig cfg;
    std::random_device rd;
    cfg.seed = rd();

    if (argc <= 1) {
        cfg.command = "help";
        return cfg;
    }

    std::string firstArg = argv[1];
    int startIdx = 1;

    if (firstArg == "stream" || firstArg == "play" || firstArg == "generate" ||
        firstArg == "export" || firstArg == "bench" || firstArg == "scales" || firstArg == "help" ||
        firstArg == "version") {
        cfg.command = firstArg;
        startIdx = 2;
    } else if (firstArg == "-h" || firstArg == "--help") {
        cfg.command = "help";
        return cfg;
    } else if (firstArg == "-v" || firstArg == "--version") {
        cfg.command = "version";
        return cfg;
    } else if (firstArg.starts_with("-")) {
        cfg.command = "stream";
        startIdx = 1;
    } else {
        throw std::invalid_argument("Unknown command '" + firstArg + "'. See 'somnia-cli --help'.");
    }

    for (int i = startIdx; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            cfg.command = "help";
            return cfg;
        }
        if (arg == "--version") {
            cfg.command = "version";
            return cfg;
        }
        if (arg == "-v" || arg == "--verbose") {
            cfg.verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            cfg.quiet = true;
        } else if (arg == "--json") {
            cfg.format = "json";
        } else if ((arg == "-s" || arg == "--scale") && i + 1 < argc) {
            std::string s = argv[++i];
            if (!ScaleEngine::isValidScaleName(s)) {
                throw std::invalid_argument("Unknown musical scale '" + s +
                                            "'. Run 'somnia-cli scales' for supported scales.");
            }
            cfg.scaleName = s;
        } else if (arg.starts_with("--scale=")) {
            std::string s = arg.substr(8);
            if (!ScaleEngine::isValidScaleName(s)) {
                throw std::invalid_argument("Unknown musical scale '" + s +
                                            "'. Run 'somnia-cli scales' for supported scales.");
            }
            cfg.scaleName = s;
        } else if ((arg == "-r" || arg == "--root") && i + 1 < argc) {
            std::string r = argv[++i];
            if (!ScaleEngine::isValidRootNote(r)) {
                throw std::invalid_argument("Invalid root note '" + r +
                                            "'. Expected note such as C, D#, Eb, F#, Bb, etc.");
            }
            cfg.rootNote = r;
        } else if (arg.starts_with("--root=")) {
            std::string r = arg.substr(7);
            if (!ScaleEngine::isValidRootNote(r)) {
                throw std::invalid_argument("Invalid root note '" + r +
                                            "'. Expected note such as C, D#, Eb, F#, Bb, etc.");
            }
            cfg.rootNote = r;
        } else if ((arg == "-b" || arg == "--bpm") && i + 1 < argc) {
            cfg.bpm = parseStrictBpm(argv[++i]);
        } else if (arg.starts_with("--bpm=")) {
            cfg.bpm = parseStrictBpm(arg.substr(6));
        } else if ((arg == "-n" || arg == "--steps") && i + 1 < argc) {
            cfg.steps = parseStrictSteps(argv[++i]);
            cfg.stepsSpecified = true;
        } else if (arg.starts_with("--steps=")) {
            cfg.steps = parseStrictSteps(arg.substr(8));
            cfg.stepsSpecified = true;
        } else if (arg == "--seed" && i + 1 < argc) {
            cfg.seed = parseStrictSeed(argv[++i]);
            cfg.seedSpecified = true;
        } else if (arg.starts_with("--seed=")) {
            cfg.seed = parseStrictSeed(arg.substr(7));
            cfg.seedSpecified = true;
        } else if (arg == "--mode" && i + 1 < argc) {
            std::string m = argv[++i];
            if (m != "melody" && m != "arpeggio" && m != "arp" && m != "chords" && m != "chord") {
                throw std::invalid_argument("Unknown play mode '" + m +
                                            "'. Supported: melody, arpeggio, chords.");
            }
            cfg.playMode = m;
        } else if (arg.starts_with("--mode=")) {
            std::string m = arg.substr(7);
            if (m != "melody" && m != "arpeggio" && m != "arp" && m != "chords" && m != "chord") {
                throw std::invalid_argument("Unknown play mode '" + m +
                                            "'. Supported: melody, arpeggio, chords.");
            }
            cfg.playMode = m;
        } else if (arg == "--meter" && i + 1 < argc) {
            std::string sig = argv[++i];
            if (sig != "4/4" && sig != "4" && sig != "3/4" && sig != "3" && sig != "7/8" &&
                sig != "7") {
                throw std::invalid_argument("Unsupported meter / time signature '" + sig +
                                            "'. Supported: 4/4, 3/4, 7/8.");
            }
            cfg.timeSig = sig;
        } else if (arg.starts_with("--meter=")) {
            std::string sig = arg.substr(8);
            if (sig != "4/4" && sig != "4" && sig != "3/4" && sig != "3" && sig != "7/8" &&
                sig != "7") {
                throw std::invalid_argument("Unsupported meter / time signature '" + sig +
                                            "'. Supported: 4/4, 3/4, 7/8.");
            }
            cfg.timeSig = sig;
        } else if (arg == "--timestep" && i + 1 < argc) {
            cfg.timeStep = parseStrictTimeStep(argv[++i]);
        } else if (arg.starts_with("--timestep=")) {
            cfg.timeStep = parseStrictTimeStep(arg.substr(11));
        } else if (arg == "--rests") {
            cfg.allowRests = true;
        } else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            cfg.format = argv[++i];
        } else if (arg.starts_with("--format=")) {
            cfg.format = arg.substr(9);
        } else if (arg == "--midi" && i + 1 < argc) {
            cfg.midiOut = argv[++i];
        } else if (arg.starts_with("--midi=")) {
            cfg.midiOut = arg.substr(7);
        } else if (arg == "--wav" && i + 1 < argc) {
            cfg.wavOut = argv[++i];
        } else if (arg.starts_with("--wav=")) {
            cfg.wavOut = arg.substr(6);
        } else if (arg == "--json-out" && i + 1 < argc) {
            cfg.jsonOut = argv[++i];
        } else if (arg.starts_with("--json-out=")) {
            cfg.jsonOut = arg.substr(11);
        } else if (arg == "--csv" && i + 1 < argc) {
            cfg.csvOut = argv[++i];
        } else if (arg.starts_with("--csv=")) {
            cfg.csvOut = arg.substr(6);
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            cfg.outputFile = argv[++i];
        } else if (arg.starts_with("--output=")) {
            cfg.outputFile = arg.substr(9);
        } else if (arg == "--no-color") {
            cfg.noColor = true;
        } else if (arg == "--no-delay") {
            cfg.realtimeDelay = false;
        } else {
            throw std::invalid_argument("Unknown argument or flag '" + arg +
                                        "'. See 'somnia-cli --help'.");
        }
    }

    return cfg;
}
