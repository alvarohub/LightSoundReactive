#pragma once

#include <stdint.h>

#include "config.h"

// Everything one light can be told to do. Each light owns a full set, so the
// two outputs behave independently.
struct LedSettings {
  enum Mode : uint8_t {
    kReactive = 0, // brightness follows the music, within this light's band
    kGlow = 1,     // slow breathing, independent of the microphone
  };

  uint8_t mode = kReactive;
  uint8_t enabled = 1; // 0 switches the output off, keeping every other setting

  // Reactive mode listens to this window only: center +/- width/2.
  // A window that covers the whole spectrum behaves as a broadband reaction.
  uint16_t centerHz = 1000;
  uint16_t widthHz = 16000;

  uint16_t attackMs = 30;   // 0 = follow instantly
  uint16_t releaseMs = 200; // exponential decay when the sound drops
  uint8_t gatePct = 4;      // level below this stays dark

  // Echo: every onset is replayed this many times, each quieter than the last.
  uint8_t echoCount = 0;        // 0 = off, up to 10
  uint16_t echoSpacingMs = 120; // time between repeats
  uint8_t echoFeedbackPct = 60; // each repeat keeps this share of the previous

  enum BeatMode : uint8_t {
    kBeatOff = 0,     // beat detection does not affect this channel
    kBeatBoost = 1,   // every beat adds a flash on top of the reactive level
    kBeatGlowSync = 2 // glow period follows the tempo, peaking on the beat
  };
  uint8_t beatMode = kBeatOff;
  uint8_t beatBoostPct = 60; // how much a beat adds in kBeatBoost

  // Glow mode
  uint16_t glowPeriodMs = 2000; // full dim -> bright -> dim cycle
  uint8_t glowMinPct = 0;
  uint8_t glowMaxPct = 100;
};

// Runtime parameters, shared by the lights, the screen and the web UI.
// All of them are clamped into their documented range, so a bad value coming
// over HTTP can never reach the PWM stage.
struct Settings {
  static constexpr uint16_t kMinGlowPeriodMs = 100;
  static constexpr uint16_t kMaxGlowPeriodMs = 5000;
  static constexpr uint16_t kMaxSmoothMs = 2000;
  static constexpr uint8_t kMaxGatePct = 50;
  static constexpr uint16_t kMinCenterHz = (uint16_t)kAudioMinHz;
  static constexpr uint16_t kMaxCenterHz = (uint16_t)kAudioMaxHz;
  static constexpr uint16_t kMinWidthHz = 40;
  static constexpr uint16_t kMaxWidthHz = 16000;

  static constexpr uint8_t kMaxEchoCount = 10;
  static constexpr uint16_t kMinEchoSpacingMs = 20;
  static constexpr uint16_t kMaxEchoSpacingMs = 500;
  static constexpr uint8_t kMaxEchoFeedbackPct = 90;

  LedSettings led[kLedCount];
  uint8_t gainDb = 0;            // shared: there is one microphone
  uint8_t beatSensitivityPct = 50; // shared: one detector for all channels
};

inline uint16_t clampU16(int32_t v, uint16_t lo, uint16_t hi) {
  if (v < (int32_t)lo) {
    return lo;
  }
  return v > (int32_t)hi ? hi : (uint16_t)v;
}

inline uint8_t clampU8(int32_t v, uint8_t lo, uint8_t hi) {
  if (v < (int32_t)lo) {
    return lo;
  }
  return v > (int32_t)hi ? hi : (uint8_t)v;
}

inline void clampLedSettings(LedSettings &l) {
  l.mode = l.mode == LedSettings::kGlow ? LedSettings::kGlow : LedSettings::kReactive;
  l.enabled = l.enabled ? 1 : 0;

  l.centerHz = clampU16(l.centerHz, Settings::kMinCenterHz, Settings::kMaxCenterHz);
  l.widthHz = clampU16(l.widthHz, Settings::kMinWidthHz, Settings::kMaxWidthHz);

  l.attackMs = clampU16(l.attackMs, 0, Settings::kMaxSmoothMs);
  l.releaseMs = clampU16(l.releaseMs, 0, Settings::kMaxSmoothMs);
  l.gatePct = clampU8(l.gatePct, 0, Settings::kMaxGatePct);

  l.echoCount = clampU8(l.echoCount, 0, Settings::kMaxEchoCount);
  l.echoSpacingMs = clampU16(l.echoSpacingMs, Settings::kMinEchoSpacingMs,
                             Settings::kMaxEchoSpacingMs);
  l.echoFeedbackPct = clampU8(l.echoFeedbackPct, 0, Settings::kMaxEchoFeedbackPct);

  l.beatMode = clampU8(l.beatMode, LedSettings::kBeatOff, LedSettings::kBeatGlowSync);
  l.beatBoostPct = clampU8(l.beatBoostPct, 0, 100);

  l.glowPeriodMs =
      clampU16(l.glowPeriodMs, Settings::kMinGlowPeriodMs, Settings::kMaxGlowPeriodMs);
  l.glowMinPct = clampU8(l.glowMinPct, 0, 100);
  l.glowMaxPct = clampU8(l.glowMaxPct, 0, 100);
  if (l.glowMinPct > l.glowMaxPct) {
    const uint8_t lo = l.glowMaxPct;
    l.glowMaxPct = l.glowMinPct;
    l.glowMinPct = lo;
  }
}

inline void clampSettings(Settings &s) {
  for (size_t i = 0; i < kLedCount; i++) {
    clampLedSettings(s.led[i]);
  }

  // The microphone only offers 0 / 6 / 12 / 18 dB.
  s.gainDb = clampU8(((s.gainDb + 3) / 6) * 6, 0, 18);
  s.beatSensitivityPct = clampU8(s.beatSensitivityPct, 0, 100);
}

// First-run values: the three channels split the spectrum, which is the point of
// an R / G / B setup -- bass on red, mids on green, highs on blue.
inline Settings defaultSettings() {
  Settings s;
  const uint16_t center[kLedCount] = {150, 1000, 4000};
  const uint16_t width[kLedCount] = {250, 1400, 5000};
  for (size_t i = 0; i < kLedCount; i++) {
    s.led[i].centerHz = center[i < 3 ? i : 0];
    s.led[i].widthHz = width[i < 3 ? i : 0];
  }
  clampSettings(s);
  return s;
}
