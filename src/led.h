#pragma once

#include <stdint.h>

#include "beat.h"
#include "settings.h"

// One PWM light.
//
// Owns its pin/channel metadata and the whole temporal model (attack, release,
// gate, glow phase). The LEDC write itself stays in main.cpp, so this class has
// no Arduino or ESP-IDF dependency and can be exercised on a host.
class Led {
public:
  void begin(int pin, uint8_t channel);

  // Advances this light one frame and returns the PWM duty to write.
  // dtMs is the real elapsed time since the previous call, which keeps the glow
  // period and the decay accurate no matter how the loop is paced. `level` is
  // the energy inside this light's frequency window, already 0..1, and `beat`
  // is what the shared beat detector reported for this frame.
  uint32_t update(const LedSettings &s, float level, const BeatInfo &beat, uint32_t dtMs);

  float brightness() const { return brightness_; } // 0..1, as displayed
  uint32_t duty() const { return duty_; }
  bool isOn() const { return duty_ > 0; }
  int pin() const { return pin_; }
  uint8_t channel() const { return channel_; }

private:
  int pin_ = -1;
  uint8_t channel_ = 0;
  float smoothed_ = 0.0f;   // reactive mode: level after attack/release
  float brightness_ = 0.0f; // final 0..1 brightness
  float phase_ = 0.0f;      // glow mode: 0..1 through the cycle
  uint32_t duty_ = 0;
  uint8_t lastMode_ = 0xFF;

  // Echo state (reactive mode only).
  uint8_t echoLeft_ = 0;      // repeats still to fire
  uint32_t echoNextMs_ = 0;   // countdown to the next repeat
  float echoAmp_ = 0.0f;      // brightness of the repeat about to fire
  uint32_t sinceOnsetMs_ = 1000000u; // refractory timer, starts "long ago"
  float onsetRef_ = 0.0f;     // level the detector re-arms below
  bool armed_ = true;
};
