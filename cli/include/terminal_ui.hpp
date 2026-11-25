#pragma once

#include <string>

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

    static ColorTheme create(bool forceNoColor = false);
    void disable();
};

class TerminalRawMode {
public:
    TerminalRawMode();
    ~TerminalRawMode();

    TerminalRawMode(const TerminalRawMode&) = delete;
    TerminalRawMode& operator=(const TerminalRawMode&) = delete;
    TerminalRawMode(TerminalRawMode&&) = delete;
    TerminalRawMode& operator=(TerminalRawMode&&) = delete;

    void restore();
    int readKey();

private:
    bool m_active = false;
#if defined(__unix__) || defined(__APPLE__)
    struct termios_holder;
    void* m_termiosState = nullptr;
#endif
};

std::string renderPitchMeter(uint16_t freq, int width = 16);
std::string renderPianoRollLane(uint8_t midiNote, int width = 24);
void printBanner(const ColorTheme& c, bool quiet = false);
