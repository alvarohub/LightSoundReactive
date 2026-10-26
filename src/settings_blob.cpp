#include "settings_blob.h"

namespace {

inline void put16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)(v >> 8);
}

inline uint16_t get16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

// mode, enabled, center(2), width(2), attack(2), release(2), gate,
// glowPeriod(2), glowMin, glowMax, echoCount, echoSpacing(2), echoFeedback,
// beatMode, beatBoost, reserved
inline void encodeLed(const LedSettings &l, uint8_t *p) {
  p[0] = l.mode;
  p[1] = l.enabled;
  put16(p + 2, l.centerHz);
  put16(p + 4, l.widthHz);
  put16(p + 6, l.attackMs);
  put16(p + 8, l.releaseMs);
  p[10] = l.gatePct;
  put16(p + 11, l.glowPeriodMs);
  p[13] = l.glowMinPct;
  p[14] = l.glowMaxPct;
  p[15] = l.echoCount;
  put16(p + 16, l.echoSpacingMs);
  p[18] = l.echoFeedbackPct;
  p[19] = l.beatMode;
  p[20] = l.beatBoostPct;
  p[21] = 0; // reserved
}

inline void decodeLed(const uint8_t *p, LedSettings &l) {
  l.mode = p[0];
  l.enabled = p[1];
  l.centerHz = get16(p + 2);
  l.widthHz = get16(p + 4);
  l.attackMs = get16(p + 6);
  l.releaseMs = get16(p + 8);
  l.gatePct = p[10];
  l.glowPeriodMs = get16(p + 11);
  l.glowMinPct = p[13];
  l.glowMaxPct = p[14];
  l.echoCount = p[15];
  l.echoSpacingMs = get16(p + 16);
  l.echoFeedbackPct = p[18];
  l.beatMode = p[19];
  l.beatBoostPct = p[20];
}

} // namespace

void encodeSettings(const Settings &s, uint8_t *out) {
  out[0] = kSettingsBlobVersion;
  out[1] = s.gainDb;
  out[2] = s.beatSensitivityPct;
  for (size_t i = 0; i < kLedCount; i++) {
    encodeLed(s.led[i], out + kSettingsHeaderSize + i * kLedBlockSize);
  }
}

bool decodeSettings(const uint8_t *in, size_t len, Settings &out) {
  if (in == nullptr || len != kSettingsBlobSize || in[0] != kSettingsBlobVersion) {
    return false;
  }

  Settings s;
  s.gainDb = in[1];
  s.beatSensitivityPct = in[2];
  for (size_t i = 0; i < kLedCount; i++) {
    decodeLed(in + kSettingsHeaderSize + i * kLedBlockSize, s.led[i]);
  }
  clampSettings(s);

  out = s;
  return true;
}
