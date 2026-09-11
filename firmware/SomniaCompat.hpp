#pragma once

#ifdef ARDUINO

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#define SOMNIA_PROGMEM PROGMEM
#define SOMNIA_READ_BYTE(addr) pgm_read_byte(addr)
#define SOMNIA_READ_WORD(addr) pgm_read_word(addr)
#else
#define SOMNIA_PROGMEM
#define SOMNIA_READ_BYTE(addr) (*(addr))
#define SOMNIA_READ_WORD(addr) (*(addr))
#endif

namespace std {
using ::floor;
using ::pow;
using ::round;
} // namespace std

namespace somnia {
template <typename T, size_t N>
struct array {
    T data[N];
    constexpr size_t size() const {
        return N;
    }
    constexpr const T& operator[](size_t i) const {
        return data[i];
    }
    T& operator[](size_t i) {
        return data[i];
    }
    constexpr const T* begin() const {
        return data;
    }
    constexpr const T* end() const {
        return data + N;
    }
    T* begin() {
        return data;
    }
    T* end() {
        return data + N;
    }
    constexpr bool empty() const {
        return N == 0;
    }
};

template <typename T>
struct span {
    const T* ptr;
    size_t len;

    constexpr span() : ptr(nullptr), len(0) {
    }
    constexpr span(const T* p, size_t l) : ptr(p), len(l) {
    }

    template <typename U, size_t N>
    constexpr span(const array<U, N>& arr) : ptr(arr.data), len(N) {
    }

    constexpr size_t size() const {
        return len;
    }
    constexpr const T& operator[](size_t i) const {
        return ptr[i];
    }
    constexpr bool empty() const {
        return len == 0;
    }
};

struct string_view {
    const char* str;
    constexpr string_view() : str("") {
    }
    constexpr string_view(const char* s) : str(s) {
    }
    constexpr const char* data() const {
        return str;
    }
    constexpr bool empty() const {
        return str == nullptr || str[0] == '\0';
    }
};

template <typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

template <typename T>
constexpr T lerp(T a, T b, T t) {
    return a + t * (b - a);
}
} // namespace somnia

namespace std_compat = somnia;

#else

#define SOMNIA_PROGMEM
#define SOMNIA_READ_BYTE(addr) (*(addr))
#define SOMNIA_READ_WORD(addr) (*(addr))

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <string_view>

namespace std_compat {
using std::array;
using std::clamp;
using std::lerp;
using std::span;
using std::string_view;
} // namespace std_compat

#endif
