#pragma once

/**
 * @file Somnia.h
 * @brief Master header for the Somnia Arduino / PlatformIO embedded library.
 *
 * Provides deterministic zero-allocation procedural audio generation,
 * 12-TET tuning lookup tables, ADSR envelopes, polyphonic mixing,
 * and hardware abstraction drivers (Buzzer, I2S, MIDI, Analog Filters, Storage).
 */

#include "adsr_envelope.hpp"
#include "analog_filter.hpp"
#include "arduino_audio_output.hpp"
#include "i2s_audio_output.hpp"
#include "pattern_engine.hpp"
#include "polyphonic_mixer.hpp"
#include "scale_engine.hpp"
#include "sequence_engine.hpp"
#include "somnia_compat.hpp"
#include "somnia_midi.hpp"
#include "somnia_storage.hpp"
#include "somnia_usb_midi.hpp"
