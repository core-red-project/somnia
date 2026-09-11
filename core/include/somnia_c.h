#ifndef SOMNIA_C_H
#define SOMNIA_C_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
    const char* label;
    uint8_t midi_note;
    uint8_t velocity;
    bool is_rest;
} somnia_c_note_event_t;

typedef void* somnia_engine_t;

somnia_engine_t somnia_create(void);
void somnia_destroy(somnia_engine_t engine);

somnia_c_note_event_t somnia_next_event(somnia_engine_t engine, uint32_t seed, uint8_t scale_type,
                                        uint8_t root_offset, uint16_t bpm, bool allow_rests,
                                        uint8_t play_mode, uint8_t time_sig);

bool somnia_export_midi(const char* filepath, uint32_t seed, uint8_t scale_type,
                        uint8_t root_offset, uint16_t bpm, size_t steps);

bool somnia_export_wav(const char* filepath, uint32_t seed, uint8_t scale_type, uint8_t root_offset,
                       uint16_t bpm, size_t steps);

const char* somnia_version(void);

#ifdef __cplusplus
}
#endif

#endif // SOMNIA_C_H
