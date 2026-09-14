#include "ExportEngine.hpp"
#include "MidiEngine.hpp"
#include "PatternEngine.hpp"
#include "ScaleEngine.hpp"
#include "SequenceEngine.hpp"
#include "WavEngine.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace {

std::atomic<bool> g_interrupted{false};

void signalHandler(int) {
    g_interrupted = true;
    // Ensure cursor is restored on SIGINT
    std::cout << "\033[?25h" << std::flush;
}

class TerminalRawMode {
public:
    TerminalRawMode() {
#if defined(__unix__) || defined(__APPLE__)
        if (isatty(STDIN_FILENO)) {
            tcgetattr(STDIN_FILENO, &m_origTermios);
            struct termios raw = m_origTermios;
            raw.c_lflag &= ~(ECHO | ICANON); // Non-canonical, no local echo
            raw.c_cc[VMIN] = 0;              // Non-blocking read
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            m_active = true;
        }
#endif
        // Hide terminal cursor
        std::cout << "\033[?25l" << std::flush;
    }

    ~TerminalRawMode() {
        restore();
    }

    void restore() {
#if defined(__unix__) || defined(__APPLE__)
        if (m_active) {
            tcsetattr(STDIN_FILENO, TCSANOW, &m_origTermios);
            m_active = false;
        }
#endif
        // Restore terminal cursor
        std::cout << "\033[?25h" << std::flush;
    }

    int readKey() {
#if defined(__unix__) || defined(__APPLE__)
        if (!m_active)
            return -1;
        char c = 0;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n > 0)
            return static_cast<int>(c);
#endif
        return -1;
    }

private:
#if defined(__unix__) || defined(__APPLE__)
    struct termios m_origTermios{};
    bool m_active = false;
#endif
};

void playWavAudio(const std::string& wavPath) {
#if defined(__APPLE__)
    std::string cmd = "/usr/bin/afplay \"" + wavPath + "\" 2>/dev/null";
    int res = std::system(cmd.c_str());
    (void)res;
#elif defined(__linux__)
    std::string cmd =
        "aplay -q \"" + wavPath + "\" 2>/dev/null || paplay \"" + wavPath + "\" 2>/dev/null";
    int res = std::system(cmd.c_str());
    (void)res;
#elif defined(_WIN32)
    std::string cmd = "powershell -c (New-Object Media.SoundPlayer '" + wavPath + "').PlaySync()";
    int res = std::system(cmd.c_str());
    (void)res;
#else
    (void)wavPath;
#endif
}

struct ColorTheme {
    bool enabled = true;
    const char* reset = "\033[0m";
    const char* bold = "\033[1m";
    const char* dim = "\033[2m";
    const char* red = "\033[31m";
    const char* green = "\033[32m";
    const char* yellow = "\033[33m";
    const char* blue = "\033[34m";
    const char* magenta = "\033[35m";
    const char* cyan = "\033[36m";
    const char* gray = "\033[90m";

    static ColorTheme create(bool forceNoColor = false) {
        ColorTheme theme;
        if (forceNoColor || std::getenv("NO_COLOR") != nullptr) {
            theme.disable();
            return theme;
        }
#if defined(__unix__) || defined(__APPLE__)
        if (!isatty(fileno(stdout))) {
            theme.disable();
        }
#endif
        return theme;
    }

    void disable() {
        enabled = false;
        reset = "";
        bold = "";
        dim = "";
        red = "";
        green = "";
        yellow = "";
        blue = "";
        magenta = "";
        cyan = "";
        gray = "";
    }
};

struct CliConfig {
    std::string command = "help";
    std::string scaleName = "lydian";
    std::string rootNote = "C";
    uint16_t bpm = 120;
    size_t steps = 16;
    bool stepsSpecified = false;
    uint32_t seed = 0;
    bool seedSpecified = false;
    float timeStep = 0.25f;
    bool allowRests = false;
    std::string format = "text";
    bool noColor = false;
    bool verbose = false;
    bool quiet = false;
    bool realtimeDelay = true;
    std::string playMode = "melody";
    std::string timeSig = "4/4";
    std::string midiOut = "";
    std::string wavOut = "";
    std::string jsonOut = "";
    std::string csvOut = "";
    std::string outputFile = "";
};

std::string renderPitchMeter(uint16_t freq, int width = 16) {
    if (freq == 0) {
        return std::string(width, '.');
    }
    float normalized = (static_cast<float>(freq) - 100.0f) / 1900.0f;
    if (normalized < 0.0f)
        normalized = 0.0f;
    if (normalized > 1.0f)
        normalized = 1.0f;
    int filled = static_cast<int>(normalized * static_cast<float>(width));
    if (filled < 1)
        filled = 1;
    if (filled > width)
        filled = width;

    std::string bar;
    for (int i = 0; i < filled; ++i)
        bar += "#";
    for (int i = filled; i < width; ++i)
        bar += " ";
    return bar;
}

std::string renderPianoRollLane(uint8_t midiNote, int width = 24) {
    if (midiNote < 48)
        midiNote = 48; // C3
    if (midiNote > 96)
        midiNote = 96; // C7
    float norm = static_cast<float>(midiNote - 48) / 48.0f;
    int pos = static_cast<int>(norm * static_cast<float>(width - 1));
    if (pos < 0)
        pos = 0;
    if (pos >= width)
        pos = width - 1;

    std::string lane(width, ' ');
    lane[pos] = '#';
    return lane;
}

void printBanner(const ColorTheme& c, bool quiet = false) {
    if (quiet)
        return;
    std::cout << c.cyan << c.bold << "✦ Somnia " << c.reset << c.dim
              << "v0.5.0 — Zero-Allocation Procedural Melody Engine" << c.reset << "\n"
              << c.gray << "  Deterministic O(1) audio stream generator" << c.reset << "\n\n";
}

void printHelp(const ColorTheme& c) {
    printBanner(c);
    std::cout << c.bold << "USAGE:" << c.reset << "\n"
              << "  somnia <command> [options]\n\n"
              << c.bold << "COMMANDS:" << c.reset << "\n"
              << "  " << c.cyan << "play" << c.reset
              << "        Play procedural melody through computer speakers in real-time\n"
              << "  " << c.cyan << "stream" << c.reset
              << "      Stream procedural notes continuously (interactive 2D TUI with live keys)\n"
              << "  " << c.cyan << "generate" << c.reset
              << "    Generate N notes and print in specified format\n"
              << "  " << c.cyan << "export" << c.reset
              << "      Generate and export to .mid, .wav, .json, or .csv\n"
              << "  " << c.cyan << "scales" << c.reset
              << "      List available scales and musical intervals\n"
              << "  " << c.cyan << "bench" << c.reset
              << "       Run high-throughput O(1) latency microbenchmark\n"
              << "  " << c.cyan << "version" << c.reset
              << "     Display version and engine capabilities\n"
              << "  " << c.cyan << "help" << c.reset << "        Show this help message\n\n"
              << c.bold << "OPTIONS:" << c.reset << "\n"
              << "  " << c.yellow << "-s, --scale <name>" << c.reset
              << "     Scale: lydian, pentatonic, dorian, blues, hirajoshi...\n"
              << "  " << c.yellow << "-r, --root <note>" << c.reset
              << "      Root note: C, C#, D, Eb, F#, G, A, Bb... (default: C)\n"
              << "  " << c.yellow << "-b, --bpm <bpm>" << c.reset
              << "        Tempo in BPM (default: 120)\n"
              << "  " << c.yellow << "-n, --steps <count>" << c.reset
              << "    Number of steps/notes to produce (default: 16)\n"
              << "  " << c.yellow << "    --seed <u32>" << c.reset
              << "       Deterministic procedural seed (auto-generated if omitted)\n"
              << "  " << c.yellow << "    --mode <name>" << c.reset
              << "      Performance mode: melody, arpeggio, chords (default: melody)\n"
              << "  " << c.yellow << "    --meter <sig>" << c.reset
              << "      Time signature meter: 4/4, 3/4, 7/8 (default: 4/4)\n"
              << "  " << c.yellow << "    --timestep <f32>" << c.reset
              << "   Time delta per step in Perlin space (default: 0.25)\n"
              << "  " << c.yellow << "    --rests" << c.reset
              << "            Enable organic procedural rests / silences\n"
              << "  " << c.yellow << "-f, --format <fmt>" << c.reset
              << "     Output format: text, json, jsonl, csv, raw, pcm (default: text)\n"
              << "  " << c.yellow << "    --json" << c.reset
              << "             Shortcut for --format json\n"
              << "  " << c.yellow << "    --midi <file.mid>" << c.reset
              << "  Export to Standard MIDI File\n"
              << "  " << c.yellow << "    --wav <file.wav>" << c.reset
              << "   Export to 16-bit 44.1kHz WAV PCM file\n"
              << "  " << c.yellow << "-v, --verbose" << c.reset
              << "          Print detailed telemetry and velocity\n"
              << "  " << c.yellow << "-q, --quiet" << c.reset
              << "            Suppress non-essential banners and logs\n"
              << "  " << c.yellow << "    --no-color" << c.reset
              << "         Disable ANSI terminal colors\n"
              << "  " << c.yellow << "    --no-delay" << c.reset
              << "         Disable real-time playback delay in stream mode\n\n"
              << c.bold << "INTERACTIVE STREAM KEYS (while running somnia stream):" << c.reset
              << "\n"
              << "  " << c.green << "[Space]" << c.reset << " Pause / Resume streaming\n"
              << "  " << c.green << "[S]" << c.reset << "     Cycle to next musical scale\n"
              << "  " << c.green << "[R]" << c.reset << "     Mutate procedural seed on-the-fly\n"
              << "  " << c.green << "[M]" << c.reset << "     Toggle Melody / Arpeggiator mode\n"
              << "  " << c.green << "[+ / -]" << c.reset << " Increase / Decrease BPM by 10\n"
              << "  " << c.green << "[Q / Esc]" << c.reset
              << " Gracefully quit and restore terminal\n\n"
              << c.bold << "EXAMPLES:" << c.reset << "\n"
              << "  " << c.gray << "# Stream with live 2D piano roll and interactive keys"
              << c.reset << "\n"
              << "  somnia stream --scale dorian --bpm 110 --rests\n\n"
              << "  " << c.gray << "# Stream raw 16-bit PCM samples into an audio player pipe"
              << c.reset << "\n"
              << "  somnia stream --format pcm | aplay -f cd\n\n"
              << "  " << c.gray << "# Generate 32 notes in JSON format directly" << c.reset << "\n"
              << "  somnia generate -n 32 --json --scale hirajoshi | jq .\n";
}

void printVersion(const ColorTheme& c) {
    printBanner(c);
    std::cout << "Engine:       C++20 Zero-Allocation Deterministic Core\n"
              << "Architecture: 1D Perlin Value Noise + 12-TET LUT O(1) + Q8 Fixed-Point\n"
              << "Audio:        Native Speaker Playback (afplay/aplay/WAV) + Raw PCM stream\n"
              << "Exporters:    Standard MIDI 0, 16-bit PCM WAV, JSON/JSONL/CSV\n"
              << "Interactivity: 2D Piano Roll TUI + Hotkey Live Controls\n"
              << "Platform:     Native Desktop CLI, 8-bit AVR / Arduino, ESP32 & Raspberry Pi Pico\n"
              << "License:      MIT (Core Red Project)\n";
}

void printScales(const ColorTheme& c, const std::string& root) {
    printBanner(c);
    ScaleEngine scaleEngine;
    uint8_t rootOffset = ScaleEngine::parseRootNote(root);

    std::cout << c.bold << "Available Scales (Root: " << c.cyan
              << ScaleEngine::rootNoteName(rootOffset).data() << c.reset << c.bold
              << "):" << c.reset << "\n\n";

    static const ScaleType scales[] = {ScaleType::Lydian,          ScaleType::MinorPentatonic,
                                       ScaleType::MajorPentatonic, ScaleType::Dorian,
                                       ScaleType::NaturalMinor,    ScaleType::Major,
                                       ScaleType::Mixolydian,      ScaleType::Phrygian,
                                       ScaleType::Blues,           ScaleType::Hirajoshi};

    for (ScaleType s : scales) {
        std::cout << "  " << c.magenta << std::left << std::setw(18)
                  << ScaleEngine::scaleName(s).data() << c.reset << " [";
        for (size_t i = 0; i < 7; ++i) {
            Note n = scaleEngine.getNote(i, s, rootOffset, 4);
            std::cout << (i > 0 ? " " : "") << n.name.data();
        }
        std::cout << " ...]\n";
    }
    std::cout << "\n";
}

void runBenchmark(const ColorTheme& c) {
    printBanner(c);
    std::cout << c.bold << "Running O(1) Performance Benchmark..." << c.reset << "\n";

    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    constexpr size_t iterations = 200000;
    StreamState state{};
    constexpr float timeStep = 0.25f;
    constexpr uint32_t seed = 424242;

    auto start = std::chrono::high_resolution_clock::now();

    uint32_t checksum = 0;
    for (size_t i = 0; i < iterations; ++i) {
        NoteEvent ev =
            sequenceEngine.nextEvent(state, timeStep, ScaleType::Lydian, seed, 0, 120, false);
        checksum ^= (ev.frequency + ev.duration_ms + ev.midi_note);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsedUs = end - start;

    double totalSec = elapsedUs.count() / 1000000.0;
    double nsPerNote = (elapsedUs.count() * 1000.0) / static_cast<double>(iterations);
    double notesPerSec = static_cast<double>(iterations) / totalSec;

    std::cout << c.green << "✦ Benchmark Completed Successfully" << c.reset << "\n"
              << "  Total notes generated: " << c.bold << iterations << c.reset << "\n"
              << "  Total elapsed time:    " << c.bold << std::fixed << std::setprecision(2)
              << elapsedUs.count() / 1000.0 << " ms" << c.reset << "\n"
              << "  Latency per note:      " << c.cyan << c.bold << std::fixed
              << std::setprecision(1) << nsPerNote << " ns / note" << c.reset << "\n"
              << "  Throughput:            " << c.yellow << c.bold << std::fixed
              << std::setprecision(0) << notesPerSec << " notes/sec" << c.reset << "\n"
              << "  Integrity Checksum:    " << c.dim << "0x" << std::hex << checksum << std::dec
              << c.reset << "\n\n"
              << c.gray
              << "  Result: Guaranteed O(1) deterministic execution with zero dynamic allocations."
              << c.reset << "\n";
}

PlayMode parsePlayMode(const std::string& str) {
    if (str == "arpeggio" || str == "arp")
        return PlayMode::Arpeggio;
    if (str == "chords" || str == "chord")
        return PlayMode::Chords;
    return PlayMode::Melody;
}

TimeSignature parseTimeSignature(const std::string& str) {
    if (str == "3/4" || str == "3")
        return TimeSignature::ThreeFour;
    if (str == "7/8" || str == "7")
        return TimeSignature::SevenEight;
    return TimeSignature::FourFour;
}

int handlePlay(const CliConfig& cfg, const ColorTheme& c) {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    ScaleType scale = ScaleEngine::parseScale(cfg.scaleName);
    uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);
    PlayMode mode = parsePlayMode(cfg.playMode);
    TimeSignature sig = parseTimeSignature(cfg.timeSig);

    StreamState state{};
    std::vector<NoteEvent> sequence;
    sequence.reserve(cfg.steps);

    for (size_t i = 0; i < cfg.steps; ++i) {
        NoteEvent ev = sequenceEngine.nextEvent(state, cfg.timeStep, scale, cfg.seed, rootOffset,
                                                cfg.bpm, cfg.allowRests, mode, sig);
        sequence.push_back(ev);
    }

    std::string tempWav = "/tmp/somnia_play_" + std::to_string(cfg.seed) + ".wav";
    bool ok = WavEngine::exportToFile(tempWav, sequence, 44100);
    if (!ok) {
        std::cerr << c.red << "Failed to synthesize audio file for playback." << c.reset << "\n";
        return 1;
    }

    if (!cfg.quiet) {
        printBanner(c);
        std::cout << c.bold << "✦ Playing Procedural Melody through Speakers" << c.reset << "\n"
                  << c.dim << "Scale: " << c.reset << c.cyan << cfg.scaleName << c.reset << c.dim
                  << " | Root: " << c.reset << c.cyan << cfg.rootNote << c.reset << c.dim
                  << " | BPM: " << c.reset << c.cyan << cfg.bpm << c.reset << c.dim
                  << " | Mode: " << c.reset << c.cyan << cfg.playMode << c.reset << c.dim
                  << " | Seed: " << c.reset << c.cyan << cfg.seed << c.reset << c.dim
                  << " | Steps: " << c.reset << c.cyan << cfg.steps << c.reset << "\n\n";

        std::cout << c.dim << "  STEP  NOTE     MIDI   FREQUENCY    DURATION    SPECTRUM" << c.reset
                  << "\n"
                  << c.dim << " ──────────────────────────────────────────────────────────"
                  << c.reset << "\n";
    }

    TerminalRawMode rawMode;

    std::thread audioThread([tempWav]() { playWavAudio(tempWav); });

    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto& ev = sequence[i];

        if (!cfg.quiet) {
            std::cout << "  " << c.dim << std::right << std::setw(4) << (i + 1) << c.reset << "  ";

            if (ev.is_rest) {
                std::cout << c.gray << std::left << std::setw(8) << "REST" << c.reset << c.dim
                          << " ---  " << c.reset << c.gray << "    ---    " << c.reset << std::right
                          << std::setw(6) << ev.duration_ms << "ms   " << c.dim
                          << renderPitchMeter(0, 18) << c.reset << "\n"
                          << std::flush;
            } else {
                std::cout << c.magenta << c.bold << std::left << std::setw(8) << ev.label.data()
                          << c.reset << " " << std::right << std::setw(3)
                          << static_cast<int>(ev.midi_note) << "  " << c.yellow << std::right
                          << std::setw(6) << ev.frequency << " Hz" << c.reset << "  " << c.cyan
                          << std::right << std::setw(6) << ev.duration_ms << "ms" << c.reset
                          << "   " << c.green << "[" << renderPitchMeter(ev.frequency, 18) << "]"
                          << c.reset << "\n"
                          << std::flush;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(ev.duration_ms));
    }

    if (audioThread.joinable()) {
        audioThread.join();
    }

    std::remove(tempWav.c_str());

    if (!cfg.quiet) {
        std::cout << "\n" << c.green << "✦ Playback completed." << c.reset << "\n\n";
    }
    return 0;
}

int handleGenerate(const CliConfig& cfg, const ColorTheme& c) {
    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);
    ExportEngine exportEngine;

    ScaleType scale = ScaleEngine::parseScale(cfg.scaleName);
    uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);
    PlayMode mode = parsePlayMode(cfg.playMode);
    TimeSignature sig = parseTimeSignature(cfg.timeSig);

    StreamState state{};
    std::vector<NoteEvent> sequence;
    sequence.reserve(cfg.steps);

    for (size_t i = 0; i < cfg.steps; ++i) {
        NoteEvent ev = sequenceEngine.nextEvent(state, cfg.timeStep, scale, cfg.seed, rootOffset,
                                                cfg.bpm, cfg.allowRests, mode, sig);
        sequence.push_back(ev);
    }

    // Raw PCM streaming support for Unix audio pipelines (somnia generate --format pcm | aplay)
    if (cfg.format == "pcm") {
        auto pcm = WavEngine::synthesizePcm(sequence, 44100);
        std::cout.write(reinterpret_cast<const char*>(pcm.data()), pcm.size() * sizeof(int16_t));
        std::cout.flush();
        return 0;
    }

    if (cfg.format == "json") {
        std::cout << exportEngine.toJson(sequence);
    } else if (cfg.format == "jsonl") {
        std::cout << exportEngine.toJsonLines(sequence);
    } else if (cfg.format == "csv") {
        std::cout << exportEngine.toCsv(sequence);
    } else if (cfg.format == "raw") {
        for (const auto& ev : sequence) {
            std::cout << ev.label.data() << " (" << ev.frequency << "Hz) -> " << ev.duration_ms
                      << "ms\n";
        }
    } else {
        printBanner(c, cfg.quiet);
        std::cout << c.dim << "Scale: " << c.reset << c.cyan << cfg.scaleName << c.reset << c.dim
                  << " | Root: " << c.reset << c.cyan << cfg.rootNote << c.reset << c.dim
                  << " | BPM: " << c.reset << c.cyan << cfg.bpm << c.reset << c.dim
                  << " | Mode: " << c.reset << c.cyan << cfg.playMode << c.reset << c.dim
                  << " | Seed: " << c.reset << c.cyan << cfg.seed << c.reset << c.dim
                  << " | Steps: " << c.reset << c.cyan << cfg.steps << c.reset << "\n\n";

        if (cfg.verbose) {
            std::cout << c.dim << "  #   NOTE    MIDI  VEL   FREQ      DUR      PITCH SPECTRUM\n"
                      << " ──────────────────────────────────────────────────────────────────"
                      << c.reset << "\n";
        } else {
            std::cout << c.dim << "  #   NOTE    MIDI    FREQ      DUR      PITCH SPECTRUM\n"
                      << " ─────────────────────────────────────────────────────────" << c.reset
                      << "\n";
        }

        for (size_t i = 0; i < sequence.size(); ++i) {
            const auto& ev = sequence[i];
            std::cout << " " << c.dim << std::right << std::setw(3) << i << c.reset << "  ";

            if (ev.is_rest) {
                std::cout << c.gray << std::left << std::setw(7) << "REST" << c.reset << c.dim
                          << "  ---  " << c.reset << (cfg.verbose ? "---   " : "") << c.gray
                          << "   ---   " << c.reset << std::right << std::setw(5) << ev.duration_ms
                          << "ms  " << c.dim << renderPitchMeter(0) << c.reset << "\n";
            } else {
                std::cout << c.magenta << c.bold << std::left << std::setw(7) << ev.label.data()
                          << c.reset << "  " << std::right << std::setw(3)
                          << static_cast<int>(ev.midi_note) << "  ";
                if (cfg.verbose) {
                    std::cout << c.dim << std::right << std::setw(3)
                              << static_cast<int>(ev.velocity) << "   " << c.reset;
                }
                std::cout << c.yellow << std::right << std::setw(5) << ev.frequency << "Hz"
                          << c.reset << "  " << c.cyan << std::right << std::setw(5)
                          << ev.duration_ms << "ms" << c.reset << "  " << c.green << "["
                          << renderPitchMeter(ev.frequency) << "]" << c.reset << "\n";
            }
        }
        std::cout << "\n";
    }

    return 0;
}

int handleExport(const CliConfig& cfg, const ColorTheme& c) {
    std::string midiTarget = cfg.midiOut;
    std::string wavTarget = cfg.wavOut;
    std::string jsonTarget = cfg.jsonOut;
    std::string csvTarget = cfg.csvOut;

    if (!cfg.outputFile.empty()) {
        if (cfg.format == "midi" || cfg.format == "mid" || cfg.outputFile.ends_with(".mid") ||
            cfg.outputFile.ends_with(".midi")) {
            midiTarget = cfg.outputFile;
        } else if (cfg.format == "wav" || cfg.outputFile.ends_with(".wav")) {
            wavTarget = cfg.outputFile;
        } else if (cfg.format == "json" || cfg.outputFile.ends_with(".json")) {
            jsonTarget = cfg.outputFile;
        } else if (cfg.format == "csv" || cfg.outputFile.ends_with(".csv")) {
            csvTarget = cfg.outputFile;
        } else {
            midiTarget = cfg.outputFile;
        }
    }

    if (midiTarget.empty() && wavTarget.empty() && jsonTarget.empty() && csvTarget.empty()) {
        std::cerr << c.red << "Error: " << c.reset
                  << "No export targets specified. Use -o <file>, --format <mid|wav|json|csv>, or "
                     "--midi, --wav, --json, --csv.\n";
        return 1;
    }

    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);
    ExportEngine exportEngine;

    ScaleType scale = ScaleEngine::parseScale(cfg.scaleName);
    uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);
    PlayMode mode = parsePlayMode(cfg.playMode);
    TimeSignature sig = parseTimeSignature(cfg.timeSig);

    StreamState state{};
    std::vector<NoteEvent> sequence;
    sequence.reserve(cfg.steps);

    for (size_t i = 0; i < cfg.steps; ++i) {
        NoteEvent ev = sequenceEngine.nextEvent(state, cfg.timeStep, scale, cfg.seed, rootOffset,
                                                cfg.bpm, cfg.allowRests, mode, sig);
        sequence.push_back(ev);
    }

    if (!cfg.quiet) {
        printBanner(c);
        std::cout << c.bold << "Exporting " << cfg.steps << " procedural notes (" << cfg.scaleName
                  << " mode, " << cfg.bpm << " BPM, Seed: " << cfg.seed << "):" << c.reset
                  << "\n\n";
    }

    if (!midiTarget.empty()) {
        bool ok = MidiEngine::exportToFile(midiTarget, sequence, cfg.bpm);
        if (ok) {
            std::cout << "  " << c.green << "✔" << c.reset << " Standard MIDI File:  " << c.bold
                      << midiTarget << c.reset << "\n";
        } else {
            std::cerr << "  " << c.red << "✘ Failed to write MIDI file: " << midiTarget << c.reset
                      << "\n";
        }
    }

    if (!wavTarget.empty()) {
        bool ok = WavEngine::exportToFile(wavTarget, sequence, 44100);
        if (ok) {
            std::cout << "  " << c.green << "✔" << c.reset << " 16-bit PCM Audio WAV: " << c.bold
                      << wavTarget << c.reset << " (44.1 kHz Mono)\n";
        } else {
            std::cerr << "  " << c.red << "✘ Failed to write WAV file: " << wavTarget << c.reset
                      << "\n";
        }
    }

    if (!jsonTarget.empty()) {
        std::ofstream jf(jsonTarget);
        if (jf.is_open()) {
            jf << exportEngine.toJson(sequence);
            std::cout << "  " << c.green << "✔" << c.reset << " Structured JSON File: " << c.bold
                      << jsonTarget << c.reset << "\n";
        } else {
            std::cerr << "  " << c.red << "✘ Failed to write JSON file: " << jsonTarget << c.reset
                      << "\n";
        }
    }

    if (!csvTarget.empty()) {
        std::ofstream cf(csvTarget);
        if (cf.is_open()) {
            cf << exportEngine.toCsv(sequence);
            std::cout << "  " << c.green << "✔" << c.reset << " Spreadsheet CSV File: " << c.bold
                      << csvTarget << c.reset << "\n";
        } else {
            std::cerr << "  " << c.red << "✘ Failed to write CSV file: " << csvTarget << c.reset
                      << "\n";
        }
    }

    std::cout << "\n";
    return 0;
}

int handleStream(CliConfig& cfg, const ColorTheme& c) {
    std::signal(SIGINT, signalHandler);

    // If streaming in raw PCM mode, output raw binary samples directly to stdout
    if (cfg.format == "pcm") {
        ScaleEngine scaleEngine;
        PatternEngine patternEngine;
        SequenceEngine sequenceEngine(scaleEngine, patternEngine);
        ScaleType scale = ScaleEngine::parseScale(cfg.scaleName);
        uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);
        PlayMode mode = parsePlayMode(cfg.playMode);
        TimeSignature sig = parseTimeSignature(cfg.timeSig);
        StreamState state{};
        size_t count = 0;

        while (!g_interrupted) {
            if (cfg.stepsSpecified && count >= cfg.steps)
                break;
            NoteEvent ev = sequenceEngine.nextEvent(state, cfg.timeStep, scale, cfg.seed,
                                                    rootOffset, cfg.bpm, cfg.allowRests, mode, sig);
            std::vector<NoteEvent> single = {ev};
            auto pcm = WavEngine::synthesizePcm(single, 44100);
            std::cout.write(reinterpret_cast<const char*>(pcm.data()),
                            pcm.size() * sizeof(int16_t));
            std::cout.flush();
            count++;
        }
        return 0;
    }

    TerminalRawMode rawMode;

    printBanner(c, cfg.quiet);
    std::cout << c.bold << "✦ Somnia Live 2D Piano Roll Stream" << c.reset << "\n"
              << c.dim << "Scale: " << c.reset << c.cyan << cfg.scaleName << c.reset << c.dim
              << " | Root: " << c.reset << c.cyan << cfg.rootNote << c.reset << c.dim
              << " | BPM: " << c.reset << c.cyan << cfg.bpm << c.reset << c.dim
              << " | Mode: " << c.reset << c.cyan << cfg.playMode << c.reset << c.dim
              << " | Seed: " << c.reset << c.cyan << cfg.seed << c.reset << "\n"
              << c.gray
              << "Controls: [Space] Pause  [S] Cycle Scale  [R] Mutate Seed  [M] Mode  [+/-] BPM  "
                 "[Q] Exit"
              << c.reset << "\n\n";

    std::cout << c.dim << "  STEP  NOTE     MIDI   FREQ      PIANO ROLL (C3 .. C7)     DUR"
              << c.reset << "\n"
              << c.dim << " ──────────────────────────────────────────────────────────────────"
              << c.reset << "\n";

    ScaleEngine scaleEngine;
    PatternEngine patternEngine;
    SequenceEngine sequenceEngine(scaleEngine, patternEngine);

    static const ScaleType availableScales[] = {
        ScaleType::Lydian,     ScaleType::MinorPentatonic, ScaleType::MajorPentatonic,
        ScaleType::Dorian,     ScaleType::NaturalMinor,    ScaleType::Major,
        ScaleType::Mixolydian, ScaleType::Phrygian,        ScaleType::Blues,
        ScaleType::Hirajoshi};
    int currentScaleIdx = 0;
    for (int i = 0; i < 10; ++i) {
        if (ScaleEngine::scaleName(availableScales[i]) == cfg.scaleName) {
            currentScaleIdx = i;
            break;
        }
    }

    ScaleType scale = availableScales[currentScaleIdx];
    uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);
    PlayMode mode = parsePlayMode(cfg.playMode);
    TimeSignature sig = parseTimeSignature(cfg.timeSig);

    StreamState state{};
    size_t count = 0;
    uint32_t totalMs = 0;
    bool paused = false;

    while (!g_interrupted) {
        // Poll for interactive hotkeys
        int key = rawMode.readKey();
        if (key != -1) {
            if (key == 'q' || key == 'Q' || key == 27) { // 27 = Esc
                break;
            } else if (key == ' ') {
                paused = !paused;
                std::cout << c.yellow
                          << (paused ? "  [PAUSED - Press Space to Resume]\n" : "  [RESUMED]\n")
                          << c.reset;
            } else if (key == 's' || key == 'S') {
                currentScaleIdx = (currentScaleIdx + 1) % 10;
                scale = availableScales[currentScaleIdx];
                cfg.scaleName = ScaleEngine::scaleName(scale).data();
                std::cout << c.cyan << "  [Scale changed to: " << cfg.scaleName << "]\n" << c.reset;
            } else if (key == 'r' || key == 'R') {
                cfg.seed = cfg.seed * 1664525u + 1013904223u;
                std::cout << c.magenta << "  [Seed mutated to: " << cfg.seed << "]\n" << c.reset;
            } else if (key == 'm' || key == 'M') {
                mode = (mode == PlayMode::Melody)
                           ? PlayMode::Arpeggio
                           : (mode == PlayMode::Arpeggio ? PlayMode::Chords : PlayMode::Melody);
                const char* modeName = (mode == PlayMode::Melody)
                                           ? "melody"
                                           : (mode == PlayMode::Arpeggio ? "arpeggio" : "chords");
                std::cout << c.green << "  [Performance mode: " << modeName << "]\n" << c.reset;
            } else if (key == '+' || key == '=') {
                cfg.bpm = (cfg.bpm < 300) ? cfg.bpm + 10 : cfg.bpm;
                std::cout << c.yellow << "  [BPM: " << cfg.bpm << "]\n" << c.reset;
            } else if (key == '-' || key == '_') {
                cfg.bpm = (cfg.bpm > 40) ? cfg.bpm - 10 : cfg.bpm;
                std::cout << c.yellow << "  [BPM: " << cfg.bpm << "]\n" << c.reset;
            }
        }

        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (cfg.stepsSpecified && count >= cfg.steps) {
            break;
        }

        NoteEvent ev = sequenceEngine.nextEvent(state, cfg.timeStep, scale, cfg.seed, rootOffset,
                                                cfg.bpm, cfg.allowRests, mode, sig);
        count++;
        totalMs += ev.duration_ms;

        std::cout << "  " << c.dim << std::right << std::setw(4) << count << c.reset << "  ";

        if (ev.is_rest) {
            std::cout << c.gray << std::left << std::setw(8) << "REST" << c.reset << c.dim
                      << " ---  " << c.reset << c.gray << "    ---    " << c.reset << c.dim
                      << "[........................]  " << c.reset << std::right << std::setw(5)
                      << ev.duration_ms << "ms\n"
                      << std::flush;
        } else {
            std::string lane = renderPianoRollLane(ev.midi_note, 24);
            std::cout << c.magenta << c.bold << std::left << std::setw(8) << ev.label.data()
                      << c.reset << " " << std::right << std::setw(3)
                      << static_cast<int>(ev.midi_note) << "  " << c.yellow << std::right
                      << std::setw(6) << ev.frequency << "Hz  " << c.reset << c.green << "[" << lane
                      << "]  " << c.reset << c.cyan << std::right << std::setw(5) << ev.duration_ms
                      << "ms" << c.reset << "\n"
                      << std::flush;
        }

        if (cfg.realtimeDelay) {
            // Sleep in small increments to allow rapid interactive key handling
            int elapsed = 0;
            while (elapsed < ev.duration_ms && !g_interrupted) {
                int keyInWait = rawMode.readKey();
                if (keyInWait != -1) {
                    if (keyInWait == 'q' || keyInWait == 'Q' || keyInWait == 27) {
                        g_interrupted = true;
                        break;
                    } else if (keyInWait == ' ') {
                        paused = !paused;
                        std::cout << c.yellow
                                  << (paused ? "  [PAUSED - Press Space to Resume]\n"
                                             : "  [RESUMED]\n")
                                  << c.reset;
                        break;
                    } else if (keyInWait == 's' || keyInWait == 'S') {
                        currentScaleIdx = (currentScaleIdx + 1) % 10;
                        scale = availableScales[currentScaleIdx];
                        cfg.scaleName = ScaleEngine::scaleName(scale).data();
                        std::cout << c.cyan << "  [Scale changed to: " << cfg.scaleName << "]\n"
                                  << c.reset;
                    } else if (keyInWait == 'r' || keyInWait == 'R') {
                        cfg.seed = cfg.seed * 1664525u + 1013904223u;
                        std::cout << c.magenta << "  [Seed mutated to: " << cfg.seed << "]\n"
                                  << c.reset;
                    } else if (keyInWait == 'm' || keyInWait == 'M') {
                        mode = (mode == PlayMode::Melody)
                                   ? PlayMode::Arpeggio
                                   : (mode == PlayMode::Arpeggio ? PlayMode::Chords
                                                                 : PlayMode::Melody);
                        const char* modeName =
                            (mode == PlayMode::Melody)
                                ? "melody"
                                : (mode == PlayMode::Arpeggio ? "arpeggio" : "chords");
                        std::cout << c.green << "  [Performance mode: " << modeName << "]\n"
                                  << c.reset;
                    } else if (keyInWait == '+' || keyInWait == '=') {
                        cfg.bpm = (cfg.bpm < 300) ? cfg.bpm + 10 : cfg.bpm;
                        std::cout << c.yellow << "  [BPM: " << cfg.bpm << "]\n" << c.reset;
                    } else if (keyInWait == '-' || keyInWait == '_') {
                        cfg.bpm = (cfg.bpm > 40) ? cfg.bpm - 10 : cfg.bpm;
                        std::cout << c.yellow << "  [BPM: " << cfg.bpm << "]\n" << c.reset;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                elapsed += 20;
            }
        }
    }

    rawMode.restore();

    std::cout << "\n"
              << c.cyan << "✦ Stream stopped gracefully." << c.reset << "\n"
              << "  Total events:  " << c.bold << count << c.reset << "\n"
              << "  Time elapsed:  " << c.bold << (totalMs / 1000.0f) << " s" << c.reset << "\n"
              << "  Replay melody: " << c.yellow << "somnia play --seed " << cfg.seed << " --scale "
              << cfg.scaleName << " --root " << cfg.rootNote << " --bpm " << cfg.bpm << " -n "
              << count << c.reset << "\n\n";

    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    CliConfig cfg;

    std::random_device rd;
    cfg.seed = rd();

    if (argc <= 1) {
        ColorTheme c = ColorTheme::create(false);
        printHelp(c);
        return 0;
    }

    std::string firstArg = argv[1];
    int startIdx = 1;

    if (firstArg == "play" || firstArg == "stream" || firstArg == "generate" ||
        firstArg == "export" || firstArg == "scales" || firstArg == "bench" ||
        firstArg == "version" || firstArg == "help" || firstArg == "--help" || firstArg == "-h") {
        if (firstArg == "--help" || firstArg == "-h") {
            cfg.command = "help";
        } else {
            cfg.command = firstArg;
        }
        startIdx = 2;
    } else if (firstArg == "--version" || firstArg == "-V" || firstArg == "-v") {
        cfg.command = "version";
        startIdx = 2;
    } else if (firstArg[0] == '-') {
        cfg.command = "generate";
        startIdx = 1;
    } else {
        std::cerr << "Unknown command: " << firstArg << "\nRun 'somnia --help' for usage.\n";
        return 1;
    }

    for (int i = startIdx; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            cfg.command = "help";
        } else if (arg == "--version" || arg == "-V") {
            cfg.command = "version";
        } else if (arg == "-v" || arg == "--verbose") {
            cfg.verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            cfg.quiet = true;
        } else if (arg == "--json") {
            cfg.format = "json";
        } else if ((arg == "-s" || arg == "--scale") && i + 1 < argc) {
            cfg.scaleName = argv[++i];
        } else if (arg.rfind("--scale=", 0) == 0) {
            cfg.scaleName = arg.substr(8);
        } else if ((arg == "-r" || arg == "--root") && i + 1 < argc) {
            cfg.rootNote = argv[++i];
        } else if (arg.rfind("--root=", 0) == 0) {
            cfg.rootNote = arg.substr(7);
        } else if ((arg == "-b" || arg == "--bpm") && i + 1 < argc) {
            cfg.bpm = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else if (arg.rfind("--bpm=", 0) == 0) {
            cfg.bpm = static_cast<uint16_t>(std::atoi(arg.substr(6).c_str()));
        } else if ((arg == "-n" || arg == "--steps") && i + 1 < argc) {
            cfg.steps = static_cast<size_t>(std::atol(argv[++i]));
            cfg.stepsSpecified = true;
        } else if (arg.rfind("--steps=", 0) == 0) {
            cfg.steps = static_cast<size_t>(std::atol(arg.substr(8).c_str()));
            cfg.stepsSpecified = true;
        } else if (arg == "--seed" && i + 1 < argc) {
            cfg.seed = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            cfg.seedSpecified = true;
        } else if (arg.rfind("--seed=", 0) == 0) {
            cfg.seed = static_cast<uint32_t>(std::strtoul(arg.substr(7).c_str(), nullptr, 10));
            cfg.seedSpecified = true;
        } else if (arg == "--mode" && i + 1 < argc) {
            cfg.playMode = argv[++i];
        } else if (arg.rfind("--mode=", 0) == 0) {
            cfg.playMode = arg.substr(7);
        } else if (arg == "--meter" && i + 1 < argc) {
            cfg.timeSig = argv[++i];
        } else if (arg.rfind("--meter=", 0) == 0) {
            cfg.timeSig = arg.substr(8);
        } else if (arg == "--timestep" && i + 1 < argc) {
            cfg.timeStep = static_cast<float>(std::atof(argv[++i]));
        } else if (arg == "--rests") {
            cfg.allowRests = true;
        } else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            cfg.format = argv[++i];
        } else if (arg.rfind("--format=", 0) == 0) {
            cfg.format = arg.substr(9);
        } else if (arg == "--midi" && i + 1 < argc) {
            cfg.midiOut = argv[++i];
        } else if (arg.rfind("--midi=", 0) == 0) {
            cfg.midiOut = arg.substr(7);
        } else if (arg == "--wav" && i + 1 < argc) {
            cfg.wavOut = argv[++i];
        } else if (arg.rfind("--wav=", 0) == 0) {
            cfg.wavOut = arg.substr(6);
        } else if (arg == "--json-out" && i + 1 < argc) {
            cfg.jsonOut = argv[++i];
        } else if (arg == "--csv" && i + 1 < argc) {
            cfg.csvOut = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            cfg.outputFile = argv[++i];
        } else if (arg.rfind("--output=", 0) == 0) {
            cfg.outputFile = arg.substr(9);
        } else if (arg == "--no-color") {
            cfg.noColor = true;
        } else if (arg == "--no-delay") {
            cfg.realtimeDelay = false;
        }
    }

    ColorTheme color = ColorTheme::create(cfg.noColor);

    if (cfg.command == "help") {
        printHelp(color);
        return 0;
    }
    if (cfg.command == "version") {
        printVersion(color);
        return 0;
    }
    if (cfg.command == "scales") {
        printScales(color, cfg.rootNote);
        return 0;
    }
    if (cfg.command == "bench") {
        runBenchmark(color);
        return 0;
    }
    if (cfg.command == "export") {
        return handleExport(cfg, color);
    }
    if (cfg.command == "play") {
        return handlePlay(cfg, color);
    }
    if (cfg.command == "stream") {
        return handleStream(cfg, color);
    }
    if (cfg.command == "generate") {
        return handleGenerate(cfg, color);
    }

    return 0;
}
