#pragma once

#include <stddef.h>
#include <stdint.h>

// ---------------------------------------------------------------------------
// Hardware configuration.
//
// Everything that depends on how the board is wired lives here, so moving a
// light to another pin is a one-line edit.
// ---------------------------------------------------------------------------

// One PWM output per light, in R / G / B order.
//
// G32 and G33 are the Grove connector pins (yellow and white wire). G26 is on
// the 8-pin HAT header -- it is NOT on the Grove port, so the third channel
// needs a wire to the header.
constexpr int kLedPins[] = {32, 33, 26};

// LEDC channels, one per light. M5Unified already owns channel 6 (red LED / IR
// emitter on G19) and channel 7 (LCD backlight), so 0..2 are used here.
constexpr uint8_t kLedChannels[] = {0, 1, 2};

constexpr size_t kLedCount = sizeof(kLedPins) / sizeof(kLedPins[0]);

static_assert(sizeof(kLedChannels) / sizeof(kLedChannels[0]) == kLedCount,
              "one LEDC channel per light");

constexpr uint32_t kLedPwmFreq = 5000;
constexpr uint8_t kLedPwmBits = 10;
constexpr uint32_t kLedPwmMax = (1u << kLedPwmBits) - 1;

// ---------------------------------------------------------------------------
// Audio capture.
// ---------------------------------------------------------------------------

constexpr uint32_t kSampleRate = 16000;
constexpr size_t kFftSamples = 512; // power of two; 512 @ 16 kHz = 31.25 Hz bins

// Usable analysis range, also the limits of the LED frequency windows.
constexpr float kAudioMinHz = 65.0f;
constexpr float kAudioMaxHz = 7600.0f;

// ---------------------------------------------------------------------------
// Light response.
// ---------------------------------------------------------------------------

// Makes the low end of the PWM range look linear to the eye.
constexpr float kLedGamma = 1.6f;

// ---------------------------------------------------------------------------
// Web UI.
// ---------------------------------------------------------------------------

// Worst-case size of the serialised state, with generous headroom over the
// measured worst case for three lights. Kept here so both the server buffer and
// the host tests use the same number.
constexpr size_t kStateJsonMaxBytes = 160 + 280 * kLedCount;
