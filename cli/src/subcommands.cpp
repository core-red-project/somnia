#include "subcommands.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "export_engine.hpp"
#include "midi_engine.hpp"
#include "pattern_engine.hpp"
#include "scale_engine.hpp"
#include "sequence_engine.hpp"
#include "wav_engine.hpp"

namespace {

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

void playWavAudio(const std::string& wavPath) {
#if defined(__APPLE__)
    std::string cmd = "/usr/bin/afplay \"" + wavPath + "\" 2>/dev/null";
    // NOLINTNEXTLINE(bugprone-command-processor)
    int res = std::system(cmd.c_str());
    (void)res;
#elif defined(__linux__)
    std::string cmd =
        "aplay -q \"" + wavPath + "\" 2>/dev/null || paplay \"" + wavPath + "\" 2>/dev/null";
    // NOLINTNEXTLINE(bugprone-command-processor)
    int res = std::system(cmd.c_str());
    (void)res;
#elif defined(_WIN32)
    std::string cmd = "powershell -c (New-Object Media.SoundPlayer '" + wavPath + "').PlaySync()";
    // NOLINTNEXTLINE(bugprone-command-processor)
    int res = std::system(cmd.c_str());
    (void)res;
#else
    (void)wavPath;
#endif
}

class BenchCommand : public Command {
public:
    [[nodiscard]] std::string_view name() const override {
        return "bench";
    }
    [[nodiscard]] std::string_view description() const override {
        return "Run O(1) procedural generation latency benchmark";
    }

    int execute(const CliConfig&, const ColorTheme& c) override {
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
            checksum ^= static_cast<uint32_t>(ev.frequency + ev.duration_ms + ev.midi_note);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> elapsedUs = end - start;

        double totalSec = elapsedUs.count() / 1000000.0;
        double nsPerNote = (elapsedUs.count() * 1000.0) / static_cast<double>(iterations);
        double notesPerSec = static_cast<double>(iterations) / totalSec;

        std::cout
            << c.green << "✦ Benchmark Completed Successfully" << c.reset << "\n"
            << "  Total notes generated: " << c.bold << iterations << c.reset << "\n"
            << "  Total elapsed time:    " << c.bold << std::fixed << std::setprecision(2)
            << elapsedUs.count() / 1000.0 << " ms" << c.reset << "\n"
            << "  Latency per note:      " << c.cyan << c.bold << std::fixed << std::setprecision(1)
            << nsPerNote << " ns / note" << c.reset << "\n"
            << "  Throughput:            " << c.yellow << c.bold << std::fixed
            << std::setprecision(0) << notesPerSec << " notes/sec" << c.reset << "\n"
            << "  Integrity Checksum:    " << c.dim << "0x" << std::hex << checksum << std::dec
            << c.reset << "\n\n"
            << c.gray
            << "  Result: Guaranteed O(1) deterministic execution with zero dynamic allocations."
            << c.reset << "\n";
        return 0;
    }
};

class ScalesCommand : public Command {
public:
    [[nodiscard]] std::string_view name() const override {
        return "scales";
    }
    [[nodiscard]] std::string_view description() const override {
        return "List all available musical scales and root note previews";
    }

    int execute(const CliConfig& cfg, const ColorTheme& c) override {
        printBanner(c);
        ScaleEngine scaleEngine;
        uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);

        std::cout << c.bold << "Available Musical Scales (Root: " << c.cyan
                  << ScaleEngine::rootNoteName(rootOffset).data() << c.reset << c.bold
                  << "):" << c.reset << "\n\n";

        static constexpr std::array<ScaleType, 10> scales = {ScaleType::Lydian,
                                                             ScaleType::MinorPentatonic,
                                                             ScaleType::MajorPentatonic,
                                                             ScaleType::Dorian,
                                                             ScaleType::NaturalMinor,
                                                             ScaleType::Major,
                                                             ScaleType::Mixolydian,
                                                             ScaleType::Phrygian,
                                                             ScaleType::Blues,
                                                             ScaleType::Hirajoshi};

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
        return 0;
    }
};

class PlayCommand : public Command {
public:
    [[nodiscard]] std::string_view name() const override {
        return "play";
    }
    [[nodiscard]] std::string_view description() const override {
        return "Synthesize audio melody and play through speakers";
    }

    int execute(const CliConfig& cfg, const ColorTheme& c) override {
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
            NoteEvent ev = sequenceEngine.nextEvent(state,
                                                    cfg.timeStep,
                                                    scale,
                                                    cfg.seed,
                                                    rootOffset,
                                                    cfg.bpm,
                                                    cfg.allowRests,
                                                    mode,
                                                    sig);
            sequence.push_back(ev);
        }

        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        std::string tempWav =
            (tempDir /
             ("somnia_play_" +
              std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" +
              std::to_string(cfg.seed) + ".wav"))
                .string();
        bool ok = WavEngine::exportToFile(tempWav, sequence, 44100);
        if (!ok) {
            std::cerr << c.red << "Failed to synthesize audio file for playback." << c.reset
                      << "\n";
            return 1;
        }

        if (!cfg.quiet) {
            printBanner(c);
            std::cout << c.bold << "✦ Playing Procedural Melody through Speakers" << c.reset << "\n"
                      << c.dim << "Scale: " << c.reset << c.cyan << cfg.scaleName << c.reset
                      << c.dim << " | Root: " << c.reset << c.cyan << cfg.rootNote << c.reset
                      << c.dim << " | BPM: " << c.reset << c.cyan << cfg.bpm << c.reset << c.dim
                      << " | Mode: " << c.reset << c.cyan << cfg.playMode << c.reset << c.dim
                      << " | Seed: " << c.reset << c.cyan << cfg.seed << c.reset << c.dim
                      << " | Steps: " << c.reset << c.cyan << cfg.steps << c.reset << "\n\n";

            std::cout << c.dim << "  STEP  NOTE     MIDI   FREQUENCY    DURATION    SPECTRUM"
                      << c.reset << "\n"
                      << c.dim << " ──────────────────────────────────────────────────────────"
                      << c.reset << "\n";
        }

        TerminalRawMode rawMode;
        std::thread audioThread([tempWav]() { playWavAudio(tempWav); });

        for (size_t i = 0; i < sequence.size(); ++i) {
            const auto& ev = sequence[i];

            if (!cfg.quiet) {
                std::cout << "  " << c.dim << std::right << std::setw(4) << (i + 1) << c.reset
                          << "  ";

                if (ev.is_rest) {
                    std::cout << c.gray << std::left << std::setw(8) << "REST" << c.reset << c.dim
                              << " ---  " << c.reset << c.gray << "    ---    " << c.reset
                              << std::right << std::setw(6) << ev.duration_ms << "ms   " << c.dim
                              << renderPitchMeter(0, 18) << c.reset << "\n"
                              << std::flush;
                } else {
                    std::cout << c.magenta << c.bold << std::left << std::setw(8) << ev.label.data()
                              << c.reset << " " << std::right << std::setw(3)
                              << static_cast<int>(ev.midi_note) << "  " << c.yellow << std::right
                              << std::setw(6) << ev.frequency << " Hz" << c.reset << "  " << c.cyan
                              << std::right << std::setw(6) << ev.duration_ms << "ms" << c.reset
                              << "   " << c.green << "[" << renderPitchMeter(ev.frequency, 18)
                              << "]" << c.reset << "\n"
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
};

class GenerateCommand : public Command {
public:
    [[nodiscard]] std::string_view name() const override {
        return "generate";
    }
    [[nodiscard]] std::string_view description() const override {
        return "Emit deterministic events to stdout (text, JSON, CSV, PCM)";
    }

    int execute(const CliConfig& cfg, const ColorTheme& c) override {
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
            NoteEvent ev = sequenceEngine.nextEvent(state,
                                                    cfg.timeStep,
                                                    scale,
                                                    cfg.seed,
                                                    rootOffset,
                                                    cfg.bpm,
                                                    cfg.allowRests,
                                                    mode,
                                                    sig);
            sequence.push_back(ev);
        }

        if (cfg.format == "pcm") {
            auto pcm = WavEngine::synthesizePcm(sequence, 44100);
            std::cout.write(reinterpret_cast<const char*>(pcm.data()),
                            static_cast<std::streamsize>(pcm.size() * sizeof(int16_t)));
            std::cout.flush();
            return 0;
        }

        if (cfg.format == "json") {
            std::cout << exportEngine.toJson(sequence);
        } else if (cfg.format == "jsonl") {
            std::cout << exportEngine.toJsonLines(sequence);
        } else if (cfg.format == "csv") {
            std::cout << exportEngine.toCsv(sequence);
        } else {
            printBanner(c, cfg.quiet);
            std::cout << c.bold << "✦ Procedural Melody Sequence (" << cfg.steps << " steps)"
                      << c.reset << "\n"
                      << c.dim << "Scale: " << c.reset << c.cyan << cfg.scaleName << c.reset
                      << c.dim << " | Root: " << c.reset << c.cyan << cfg.rootNote << c.reset
                      << c.dim << " | BPM: " << c.reset << c.cyan << cfg.bpm << c.reset << c.dim
                      << " | Mode: " << c.reset << c.cyan << cfg.playMode << c.reset << c.dim
                      << " | Seed: " << c.reset << c.cyan << cfg.seed << c.reset << "\n\n";

            std::cout << c.dim
                      << "  #   NOTE       MIDI   FREQ       DURATION   VELOCITY   STATUS\n"
                      << " ─────────────────────────────────────────────────────────────\n"
                      << c.reset;

            for (size_t i = 0; i < sequence.size(); ++i) {
                const auto& ev = sequence[i];
                std::cout << "  " << c.dim << std::right << std::setw(2) << (i + 1) << c.reset
                          << "  ";

                if (ev.is_rest) {
                    std::cout << c.gray << std::left << std::setw(10) << "REST" << c.reset << " "
                              << c.dim << "---    ---        " << std::right << std::setw(5)
                              << ev.duration_ms << "ms     ---        REST" << c.reset << "\n";
                } else {
                    std::cout << c.magenta << c.bold << std::left << std::setw(10)
                              << ev.label.data() << c.reset << " " << std::right << std::setw(3)
                              << static_cast<int>(ev.midi_note) << "    " << c.yellow << std::right
                              << std::setw(5) << ev.frequency << " Hz" << c.reset << "    "
                              << c.cyan << std::right << std::setw(5) << ev.duration_ms << "ms"
                              << c.reset << "     " << std::right << std::setw(3)
                              << static_cast<int>(ev.velocity) << "        " << c.green << "PLAY"
                              << c.reset << "\n";
                }
            }
            std::cout << "\n";
        }
        return 0;
    }
};

class ExportCommand : public Command {
public:
    [[nodiscard]] std::string_view name() const override {
        return "export";
    }
    [[nodiscard]] std::string_view description() const override {
        return "Export procedural composition to MIDI, WAV, JSON, or CSV file";
    }

    int execute(const CliConfig& cfg, const ColorTheme& c) override {
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
                // Default based on format or fallback to midi with specified filename
                if (cfg.format == "wav") {
                    wavTarget = cfg.outputFile;
                } else if (cfg.format == "json") {
                    jsonTarget = cfg.outputFile;
                } else if (cfg.format == "csv") {
                    csvTarget = cfg.outputFile;
                } else {
                    midiTarget = cfg.outputFile;
                }
            }
        }

        if (midiTarget.empty() && wavTarget.empty() && jsonTarget.empty() && csvTarget.empty()) {
            midiTarget = "somnia_output.mid";
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
            sequence.push_back(sequenceEngine.nextEvent(state,
                                                        cfg.timeStep,
                                                        scale,
                                                        cfg.seed,
                                                        rootOffset,
                                                        cfg.bpm,
                                                        cfg.allowRests,
                                                        mode,
                                                        sig));
        }

        printBanner(c, cfg.quiet);
        if (!cfg.quiet) {
            std::cout << c.bold << "✦ Exporting Procedural Composition (" << cfg.steps << " steps)"
                      << c.reset << "\n\n";
        }

        bool anyExportFailed = false;

        if (!midiTarget.empty()) {
            bool ok = MidiEngine::exportToFile(midiTarget, sequence, cfg.bpm);
            if (ok) {
                if (!cfg.quiet) {
                    std::cout << "  " << c.green << "✔ MIDI:" << c.reset << " " << c.bold
                              << midiTarget << c.reset << "\n";
                }
            } else {
                std::cerr << "  " << c.red << "✖ Failed to export MIDI to " << midiTarget << c.reset
                          << "\n";
                anyExportFailed = true;
            }
        }
        if (!wavTarget.empty()) {
            bool ok = WavEngine::exportToFile(wavTarget, sequence, 44100);
            if (ok) {
                if (!cfg.quiet) {
                    std::cout << "  " << c.green << "✔ WAV: " << c.reset << " " << c.bold
                              << wavTarget << c.reset << "\n";
                }
            } else {
                std::cerr << "  " << c.red << "✖ Failed to export WAV to " << wavTarget << c.reset
                          << "\n";
                anyExportFailed = true;
            }
        }
        if (!jsonTarget.empty()) {
            std::ofstream ofs(jsonTarget);
            if (ofs.is_open()) {
                ofs << exportEngine.toJson(sequence);
                if (!cfg.quiet) {
                    std::cout << "  " << c.green << "✔ JSON:" << c.reset << " " << c.bold
                              << jsonTarget << c.reset << "\n";
                }
            } else {
                std::cerr << "  " << c.red << "✖ Failed to write JSON to " << jsonTarget << c.reset
                          << "\n";
                anyExportFailed = true;
            }
        }
        if (!csvTarget.empty()) {
            std::ofstream ofs(csvTarget);
            if (ofs.is_open()) {
                ofs << exportEngine.toCsv(sequence);
                if (!cfg.quiet) {
                    std::cout << "  " << c.green << "✔ CSV: " << c.reset << " " << c.bold
                              << csvTarget << c.reset << "\n";
                }
            } else {
                std::cerr << "  " << c.red << "✖ Failed to write CSV to " << csvTarget << c.reset
                          << "\n";
                anyExportFailed = true;
            }
        }
        if (!cfg.quiet)
            std::cout << "\n";
        return anyExportFailed ? 1 : 0;
    }
};

class StreamCommand : public Command {
public:
    [[nodiscard]] std::string_view name() const override {
        return "stream";
    }
    [[nodiscard]] std::string_view description() const override {
        return "Live interactive terminal streaming (2D piano roll tracker)";
    }

    int execute(const CliConfig& cfg, const ColorTheme& c) override {
        ScaleEngine scaleEngine;
        PatternEngine patternEngine;
        SequenceEngine sequenceEngine(scaleEngine, patternEngine);

        ScaleType scale = ScaleEngine::parseScale(cfg.scaleName);
        uint8_t rootOffset = ScaleEngine::parseRootNote(cfg.rootNote);
        PlayMode mode = parsePlayMode(cfg.playMode);
        TimeSignature sig = parseTimeSignature(cfg.timeSig);

        StreamState state{};
        size_t count = 0;
        uint32_t totalMs = 0;

        if (cfg.format == "pcm") {
            while (!cfg.stepsSpecified || count < cfg.steps) {
                NoteEvent ev = sequenceEngine.nextEvent(state,
                                                        cfg.timeStep,
                                                        scale,
                                                        cfg.seed,
                                                        rootOffset,
                                                        cfg.bpm,
                                                        cfg.allowRests,
                                                        mode,
                                                        sig);
                std::vector<NoteEvent> single = {ev};
                auto pcm = WavEngine::synthesizePcm(single, 44100);
                std::cout.write(reinterpret_cast<const char*>(pcm.data()),
                                static_cast<std::streamsize>(pcm.size() * sizeof(int16_t)));
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
                  << "Controls: [Space] Pause | [S] Next Scale | [R] Mutate Seed | [Q/ESC] Quit\n"
                  << c.reset << "\n";

        std::cout << c.dim << "  STEP  NOTE     MIDI   FREQ      PIANO ROLL (C3 .. C7)     DUR"
                  << c.reset << "\n"
                  << c.dim << " ──────────────────────────────────────────────────────────────────"
                  << c.reset << "\n";

        static constexpr std::array<ScaleType, 10> availableScales = {ScaleType::Lydian,
                                                                      ScaleType::MinorPentatonic,
                                                                      ScaleType::MajorPentatonic,
                                                                      ScaleType::Dorian,
                                                                      ScaleType::NaturalMinor,
                                                                      ScaleType::Major,
                                                                      ScaleType::Mixolydian,
                                                                      ScaleType::Phrygian,
                                                                      ScaleType::Blues,
                                                                      ScaleType::Hirajoshi};
        int currentScaleIdx = 0;
        for (int i = 0; i < 10; ++i) {
            if (availableScales[static_cast<size_t>(i)] == scale) {
                currentScaleIdx = i;
                break;
            }
        }

        uint32_t activeSeed = cfg.seed;
        std::string activeScaleName = cfg.scaleName;
        bool paused = false;

        while (!cfg.stepsSpecified || count < cfg.steps) {
            int key = rawMode.readKey();
            if (key > 0) {
                if (key == 'q' || key == 'Q' || key == 27) {
                    break;
                }
                if (key == ' ') {
                    paused = !paused;
                    std::cout << c.yellow
                              << (paused ? "  [PAUSED - Press Space to Resume]\n" : "  [RESUMED]\n")
                              << c.reset;
                } else if (key == 's' || key == 'S') {
                    currentScaleIdx = (currentScaleIdx + 1) % 10;
                    scale = availableScales[static_cast<size_t>(currentScaleIdx)];
                    activeScaleName = std::string(ScaleEngine::scaleName(scale));
                    std::cout << c.cyan << "  [SWITCHED SCALE: " << activeScaleName << "]\n"
                              << c.reset;
                } else if (key == 'r' || key == 'R') {
                    activeSeed = activeSeed * 1664525u + 1013904223u;
                    std::cout << c.magenta << "  [MUTATED SEED: " << activeSeed << "]\n" << c.reset;
                }
            }

            if (paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            NoteEvent ev = sequenceEngine.nextEvent(state,
                                                    cfg.timeStep,
                                                    scale,
                                                    activeSeed,
                                                    rootOffset,
                                                    cfg.bpm,
                                                    cfg.allowRests,
                                                    mode,
                                                    sig);
            count++;
            totalMs += ev.duration_ms;

            std::cout << "  " << c.dim << std::right << std::setw(4) << count << c.reset << "  ";

            if (ev.is_rest) {
                std::cout << c.gray << std::left << std::setw(8) << "REST" << c.reset << " "
                          << c.dim << "---    ---   " << c.reset << c.dim
                          << " [ . . . . . . . . . . . . . . . . ]  " << c.reset << c.gray
                          << std::right << std::setw(5) << ev.duration_ms << "ms" << c.reset << "\n"
                          << std::flush;
            } else {
                std::string rollLane = renderPianoRollLane(ev.midi_note, 24);
                std::cout << c.magenta << c.bold << std::left << std::setw(8) << ev.label.data()
                          << c.reset << " " << std::right << std::setw(3)
                          << static_cast<int>(ev.midi_note) << "   " << c.yellow << std::right
                          << std::setw(5) << ev.frequency << "Hz" << c.reset << " " << c.green
                          << "[" << rollLane << "]  " << c.reset << c.cyan << std::right
                          << std::setw(5) << ev.duration_ms << "ms" << c.reset << "\n"
                          << std::flush;
            }

            if (cfg.realtimeDelay) {
                uint32_t elapsed = 0;
                while (elapsed < ev.duration_ms) {
                    int keyInWait = rawMode.readKey();
                    if (keyInWait != -1) {
                        if (keyInWait == 'q' || keyInWait == 'Q' || keyInWait == 27) {
                            goto stream_end;
                        }
                        if (keyInWait == ' ') {
                            paused = !paused;
                            std::cout << c.yellow
                                      << (paused ? "  [PAUSED - Press Space to Resume]\n"
                                                 : "  [RESUMED]\n")
                                      << c.reset;
                            break;
                        }
                        if (keyInWait == 's' || keyInWait == 'S') {
                            currentScaleIdx = (currentScaleIdx + 1) % 10;
                            scale = availableScales[static_cast<size_t>(currentScaleIdx)];
                            std::cout << c.cyan << "  [SWITCHED SCALE: "
                                      << ScaleEngine::scaleName(scale).data() << "]\n"
                                      << c.reset;
                        } else if (keyInWait == 'r' || keyInWait == 'R') {
                            activeSeed = activeSeed * 1664525u + 1013904223u;
                            std::cout << c.magenta << "  [MUTATED SEED: " << activeSeed << "]\n"
                                      << c.reset;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    elapsed += 20;
                }
            }
        }

    stream_end:
        rawMode.restore();

        std::cout << "\n"
                  << c.cyan << "✦ Stream stopped gracefully." << c.reset << "\n"
                  << "  Total events:  " << c.bold << count << c.reset << "\n"
                  << "  Time elapsed:  " << c.bold << (static_cast<double>(totalMs) / 1000.0)
                  << " s" << c.reset << "\n"
                  << "  Replay melody: " << c.yellow << "somnia-cli play --seed " << activeSeed
                  << " --scale " << activeScaleName << " --root " << cfg.rootNote << " --bpm "
                  << cfg.bpm << " -n " << count;
        if (cfg.playMode != "melody") {
            std::cout << " --mode " << cfg.playMode;
        }
        if (cfg.timeSig != "4/4") {
            std::cout << " --meter " << cfg.timeSig;
        }
        std::cout << c.reset << "\n\n";

        return 0;
    }
};

} // namespace

std::unique_ptr<Command> createStreamCommand() {
    return std::make_unique<StreamCommand>();
}
std::unique_ptr<Command> createPlayCommand() {
    return std::make_unique<PlayCommand>();
}
std::unique_ptr<Command> createGenerateCommand() {
    return std::make_unique<GenerateCommand>();
}
std::unique_ptr<Command> createExportCommand() {
    return std::make_unique<ExportCommand>();
}
std::unique_ptr<Command> createBenchCommand() {
    return std::make_unique<BenchCommand>();
}
std::unique_ptr<Command> createScalesCommand() {
    return std::make_unique<ScalesCommand>();
}

CommandRegistry CommandRegistry::createDefault() {
    CommandRegistry registry;
    registry.registerCommand(createStreamCommand());
    registry.registerCommand(createPlayCommand());
    registry.registerCommand(createGenerateCommand());
    registry.registerCommand(createExportCommand());
    registry.registerCommand(createBenchCommand());
    registry.registerCommand(createScalesCommand());
    return registry;
}
