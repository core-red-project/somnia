#pragma once

#include <cstdint>
#include <string>

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
    bool parseError = false;
    std::string errorMessage = "";
};

CliConfig parseCommandLine(int argc, char* argv[]);
