#pragma once

#ifdef ARDUINO
#include <Arduino.h>
#if defined(__has_include)
#if __has_include(<EEPROM.h>)
#include <EEPROM.h>
#define SOMNIA_HAS_EEPROM 1
#endif
#elif !defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_MBED)
#include <EEPROM.h>
#define SOMNIA_HAS_EEPROM 1
#endif
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#else
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#endif

// Binary schema for persistent configuration in non-volatile memory (EEPROM / NVS)
struct SomniaConfig {
    static constexpr uint32_t MAGIC = 0x534F4D4EU; // "SOMN" in ASCII
    static constexpr uint8_t SCHEMA_VERSION = 1;

    uint32_t magic = MAGIC;
    uint8_t version = SCHEMA_VERSION;
    uint8_t defaultScale = 0;
    uint16_t defaultBpm = 120;
    uint32_t savedSeed = 54321;
    uint8_t checksum = 0;

    // Fast 8-bit rotating XOR checksum over config payload
    [[nodiscard]] uint8_t calculateChecksum() const noexcept {
        uint8_t c = 0x55;
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(this);
        // Hash all bytes except the trailing checksum member
        const size_t payloadSize = offsetof(SomniaConfig, checksum);
        for (size_t i = 0; i < payloadSize; ++i) {
            c ^= ptr[i];
            c = static_cast<uint8_t>((c << 1) | (c >> 7));
        }
        return c;
    }

    void updateChecksum() noexcept {
        checksum = calculateChecksum();
    }

    [[nodiscard]] bool isValid() const noexcept {
        return (magic == MAGIC) && (version == SCHEMA_VERSION) && (checksum == calculateChecksum());
    }
};

// Generic interface for non-volatile hardware storage backends
class StorageBackend {
public:
    virtual ~StorageBackend() = default;
    virtual bool read(size_t offset, void* dest, size_t length) = 0;
    virtual bool write(size_t offset, const void* src, size_t length) = 0;
};

// In-Memory storage backend for unit testing and desktop simulation
template <size_t Capacity = 256>
class MemoryStorageBackend : public StorageBackend {
public:
    MemoryStorageBackend() {
        clear();
    }

    bool read(size_t offset, void* dest, size_t length) override {
        if (offset + length > Capacity || dest == nullptr) {
            return false;
        }
#ifdef ARDUINO
        memcpy(dest, &m_storage[offset], length);
#else
        std::memcpy(dest, &m_storage[offset], length);
#endif
        return true;
    }

    bool write(size_t offset, const void* src, size_t length) override {
        if (offset + length > Capacity || src == nullptr) {
            return false;
        }
#ifdef ARDUINO
        memcpy(&m_storage[offset], src, length);
#else
        std::memcpy(&m_storage[offset], src, length);
#endif
        return true;
    }

    void clear() {
#ifdef ARDUINO
        memset(m_storage, 0xFF, Capacity);
#else
        m_storage.fill(0xFF);
#endif
    }

private:
#ifdef ARDUINO
    uint8_t m_storage[Capacity];
#else
    std::array<uint8_t, Capacity> m_storage{};
#endif
};

#ifdef ARDUINO
#if defined(SOMNIA_HAS_EEPROM)
// Hardware EEPROM backend for microcontrollers with wear-leveling update()
class EepromStorageBackend : public StorageBackend {
public:
    void begin(size_t size = sizeof(SomniaConfig)) {
#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) ||                      \
    defined(ARDUINO_RASPBERRY_PI_PICO)
        EEPROM.begin(size);
#else
        (void)size;
#endif
    }

    bool read(size_t offset, void* dest, size_t length) override {
        auto* byteDest = static_cast<uint8_t*>(dest);
        for (size_t i = 0; i < length; ++i) {
            byteDest[i] = EEPROM.read(static_cast<int>(offset + i));
        }
        return true;
    }

    bool write(size_t offset, const void* src, size_t length) override {
        const auto* byteSrc = static_cast<const uint8_t*>(src);
        for (size_t i = 0; i < length; ++i) {
#if defined(ESP32)
            EEPROM.write(static_cast<int>(offset + i), byteSrc[i]);
#else
            // EEPROM.update writes only when byte value changes, extending cell lifetime
            EEPROM.update(static_cast<int>(offset + i), byteSrc[i]);
#endif
        }
#if defined(ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(PICO_RP2040) ||                      \
    defined(ARDUINO_RASPBERRY_PI_PICO)
        EEPROM.commit();
#endif
        return true;
    }
};
#else
// In-memory fallback for platforms lacking hardware EEPROM emulation (e.g. RP2040 official Mbed
// core)
class EepromStorageBackend : public MemoryStorageBackend<256> {
public:
    void begin(size_t /*size*/ = sizeof(SomniaConfig)) noexcept {
    }
};
#endif
#endif

// Storage manager handling serialization, validation, and safe persistence
class StorageManager {
public:
    explicit StorageManager(StorageBackend& backend, size_t baseOffset = 0) noexcept
        : m_backend(backend), m_baseOffset(baseOffset) {
    }

    bool load(SomniaConfig& config) noexcept {
        SomniaConfig loaded{};
        if (!m_backend.read(m_baseOffset, &loaded, sizeof(SomniaConfig))) {
            return false;
        }
        if (!loaded.isValid()) {
            return false;
        }
        config = loaded;
        return true;
    }

    bool save(SomniaConfig& config) noexcept {
        config.updateChecksum();
        return m_backend.write(m_baseOffset, &config, sizeof(SomniaConfig));
    }

private:
    StorageBackend& m_backend;
    size_t m_baseOffset = 0;
};
