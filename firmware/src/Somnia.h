#pragma once

/**
 * @file Somnia.h
 * @brief Master header for the Somnia Arduino / PlatformIO embedded library.
 *
 * Provides deterministic zero-allocation procedural audio generation,
 * 12-TET tuning lookup tables, ADSR envelopes, polyphonic mixing,
 * and hardware abstraction drivers (Buzzer, I2S, MIDI, Analog Filters, Storage).
 */

#include "somnia_compat.hpp"
#include "scale_engine.hpp"
#include "pattern_engine.hpp"
#include "sequence_engine.hpp"
#include "adsr_envelope.hpp"
#include "polyphonic_mixer.hpp"
#include "analog_filter.hpp"
#include "arduino_audio_output.hpp"
#include "i2s_audio_output.hpp"
#include "somnia_midi.hpp"
#include "somnia_storage.hpp"
#include "somnia_usb_midi.hpp"
