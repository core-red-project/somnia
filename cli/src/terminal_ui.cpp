#include "terminal_ui.hpp"

#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <conio.h>
#include <io.h>
#include <windows.h>
#endif

namespace {
TerminalRawMode* g_activeRawMode = nullptr;

extern "C" void handleSigint(int sig) {
    if (g_activeRawMode != nullptr) {
        g_activeRawMode->restore();
    }
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}
} // namespace

ColorTheme ColorTheme::create(bool forceNoColor) {
    ColorTheme theme;
    if (forceNoColor || std::getenv("NO_COLOR") != nullptr) {
        theme.disable();
        return theme;
    }
#if defined(__unix__) || defined(__APPLE__)
    if (isatty(fileno(stdout)) == 0) {
        theme.disable();
    }
#elif defined(_WIN32)
    if (_isatty(_fileno(stdout)) == 0) {
        theme.disable();
    }
#endif
    return theme;
}

void ColorTheme::disable() {
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

#if defined(__unix__) || defined(__APPLE__)
struct TerminalRawMode::termios_holder {
    struct termios orig{};
};
#endif

TerminalRawMode::TerminalRawMode() {
    g_activeRawMode = this;
    std::signal(SIGINT, handleSigint);
#if defined(SIGTERM)
    std::signal(SIGTERM, handleSigint);
#endif
#if defined(__unix__) || defined(__APPLE__)
    if (isatty(STDIN_FILENO) != 0) {
        auto* holder = new termios_holder();
        tcgetattr(STDIN_FILENO, &holder->orig);
        struct termios raw = holder->orig;
        raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        m_termiosState = holder;
        m_active = true;
    }
#elif defined(_WIN32)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
    m_active = true;
#endif
    std::cout << "\033[?25l" << std::flush;
}

TerminalRawMode::~TerminalRawMode() {
    restore();
#if defined(__unix__) || defined(__APPLE__)
    delete static_cast<termios_holder*>(m_termiosState);
    m_termiosState = nullptr;
#endif
}

void TerminalRawMode::restore() {
#if defined(__unix__) || defined(__APPLE__)
    if (m_active && m_termiosState != nullptr) {
        auto* holder = static_cast<termios_holder*>(m_termiosState);
        tcsetattr(STDIN_FILENO, TCSANOW, &holder->orig);
        m_active = false;
    }
#elif defined(_WIN32)
    m_active = false;
#endif
    if (g_activeRawMode == this) {
        g_activeRawMode = nullptr;
    }
    std::cout << "\033[?25h" << std::flush;
}

int TerminalRawMode::readKey() {
#if defined(__unix__) || defined(__APPLE__)
    if (!m_active)
        return -1;
    char c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0)
        return -1;

    // Check if this is the start of an ANSI escape sequence (e.g. arrow keys: ESC [ A..D)
    if (c == 27) {
        char seq[4] = {0};
        // Use non-blocking read to see if more bytes follow immediately
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        ssize_t n2 = read(STDIN_FILENO, &seq[0], 1);
        if (n2 > 0) {
            ssize_t n3 = read(STDIN_FILENO, &seq[1], 1);
            (void)n3;
            fcntl(STDIN_FILENO, F_SETFL, flags);
            // This was an escape sequence (such as arrow key or function key), consume and ignore
            return -2;
        }
        fcntl(STDIN_FILENO, F_SETFL, flags);
        // Lone ESC key pressed by user
        return 27;
    }

    return static_cast<int>(c);
#elif defined(_WIN32)
    if (_kbhit() != 0) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            // Extended key prefix on Windows (arrow keys, function keys)
            _getch(); // consume second byte
            return -2;
        }
        return ch;
    }
#endif
    return -1;
}

std::string renderPitchMeter(uint16_t freq, int width) {
    if (freq == 0) {
        // NOLINTNEXTLINE(modernize-return-braced-init-list)
        return std::string(static_cast<size_t>(width), '.');
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
        bar += '#';
    for (int i = filled; i < width; ++i)
        bar += ' ';
    return bar;
}

std::string renderPianoRollLane(uint8_t midiNote, int width) {
    if (midiNote < 48)
        midiNote = 48; // C3
    if (midiNote > 96)
        midiNote = 96; // C7

    int pos = static_cast<int>(((midiNote - 48) * (width - 1)) / (96 - 48));
    std::string lane(static_cast<size_t>(width), ' ');
    for (size_t i = 0; i < lane.size(); i += 6) {
        lane[i] = '|';
    }
    if (pos >= 0 && static_cast<size_t>(pos) < lane.size()) {
        lane[static_cast<size_t>(pos)] = 'o';
    }
    return lane;
}

void printBanner(const ColorTheme& c, bool quiet) {
    if (quiet)
        return;
    std::cout << c.magenta << c.bold << "\n"
              << "  ███████╗ ██████╗ ███╗   ███╗███╗   ██╗██╗ █████╗ \n"
              << "  ██╔════╝██╔═══██╗████╗ ████║████╗  ██║██║██╔══██╗\n"
              << "  ███████╗██║   ██║██╔████╔██║██╔██╗ ██║██║███████║\n"
              << "  ╚════██║██║   ██║██║╚██╔╝██║██║╚██╗██║██║██╔══██║\n"
              << "  ███████║╚██████╔╝██║ ╚═╝ ██║██║ ╚████║██║██║  ██║\n"
              << "  ╚══════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═══╝╚═╝╚═╝  ╚═╝\n"
              << c.reset << c.dim << "  Procedural Oníric Ambient Synthesizer Core v0.6.0\n"
              << "  Deterministic O(1) Zero-Heap Audio Engine\n"
              << c.reset << "\n";
}
