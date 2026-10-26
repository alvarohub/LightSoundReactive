#include <M5Unified.h>

#include <math.h>
#include <stdio.h>

#include "audio_fft.h"
#include "beat.h"
#include "config.h"
#include "led.h"
#include "settings.h"
#include "store.h"
#include "webui.h"

namespace {

// ---- Screen layout (240 x 135 in landscape) -------------------------------
// Vertical positions are derived from the panel height in setup(), so the
// layout also holds if the rotation or the panel differs.
constexpr int kBarTop = 12;
constexpr int kBandRowPitch = 3;
constexpr int kMeterH = 4;
constexpr int kMeterX = 14;
int hudTopY = 2;
int barBottom = 93;
int bandRowY = 94;
int meterY[kLedCount] = {};
int hudBottomY = 125;
int apLabelX = 226;
int meterWidth = 224;

// M5GFX hands colour values straight to the panel, and this panel writes 16-bit
// RGB565 (see lgfx's TFT_RED = 0xF800), so colours are built in that format.
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr uint16_t kBlack = 0x0000;
constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kGrey = 0x7BEF;
constexpr uint16_t kApOn = rgb565(0, 255, 120);

// Colours matching the R / G / B hardware, used for each light's output meter
// and its band marker. Indexed with a palette so the UI keeps working if another
// channel is added later.
constexpr uint16_t kLedPalette[] = {rgb565(255, 60, 60), rgb565(60, 255, 90),
                                    rgb565(80, 150, 255)};

inline uint16_t ledColor(size_t i) {
  return kLedPalette[i % (sizeof(kLedPalette) / sizeof(kLedPalette[0]))];
}

SpectrumAnalyzer analyzer;
BeatDetector beat_;
Led leds[kLedCount];
Settings settings;
SettingsStore store;
WebUi web;

int prevBar[SpectrumAnalyzer::kBands] = {};
int prevMeter[kLedCount] = {};
int prevBandLo[kLedCount] = {};
int prevBandHi[kLedCount] = {};
uint8_t prevBandOn[kLedCount] = {};
int bandWidth = 8;
int barWidth = 7;
uint32_t lastFrameUs = 0;
uint32_t lastHud = 0;
uint32_t lastStats = 0;
uint8_t syncedGainDb = 0xFF;

// Writing NVS on every slider move would wear the flash, so changes are saved
// once they have been quiet for a moment.
constexpr uint32_t kSaveDelayMs = 800;
bool settingsDirty = false;
uint32_t settingsChangedAt = 0;

void noteSettingsChanged() {
  settingsDirty = true;
  settingsChangedAt = millis();
}

void pwmInit(size_t i) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(kLedPins[i], kLedPwmFreq, kLedPwmBits);
#else
  ledcSetup(kLedChannels[i], kLedPwmFreq, kLedPwmBits);
  ledcAttachPin(kLedPins[i], kLedChannels[i]);
#endif
}

void pwmWrite(size_t i, uint32_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(kLedPins[i], duty);
#else
  ledcWrite(kLedChannels[i], duty);
#endif
}

// Green at the bottom, red at the top, so a bar's colour depends on its height.
uint16_t barColor(int y) {
  const int span = barBottom - kBarTop;
  float t = (float)(barBottom - y) / (float)span;
  t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);

  uint8_t r, g;
  if (t < 0.5f) {
    r = (uint8_t)(255.0f * t * 2.0f);
    g = 255;
  } else {
    r = 255;
    g = (uint8_t)(255.0f * (2.0f - t * 2.0f));
  }
  return rgb565(r, g, 0);
}

// Only the difference between frames is painted, which keeps the bars free of
// flicker and keeps the SPI traffic low enough to stay at ~30 fps.
void drawSpectrum(const float *bands) {
  const int span = barBottom - kBarTop;
  for (size_t b = 0; b < SpectrumAnalyzer::kBands; b++) {
    int h = (int)(bands[b] * (float)span + 0.5f);
    if (h > span) {
      h = span;
    }
    const int x = (int)b * bandWidth;
    if (h > prevBar[b]) {
      const int top = barBottom - h;
      M5.Display.fillRect(x, top, barWidth, h - prevBar[b], barColor(top));
    } else if (h < prevBar[b]) {
      M5.Display.fillRect(x, barBottom - prevBar[b], barWidth, prevBar[b] - h, kBlack);
    }
    prevBar[b] = h;
  }
}

// Marks which part of the spectrum each light actually listens to.
void drawBandMarkers() {
  for (size_t i = 0; i < kLedCount; i++) {
    const LedSettings &s = settings.led[i];
    float lo = (float)s.centerHz - (float)s.widthHz * 0.5f;
    float hi = (float)s.centerHz + (float)s.widthHz * 0.5f;
    if (lo < kAudioMinHz) {
      lo = kAudioMinHz;
    }
    if (hi > kAudioMaxHz) {
      hi = kAudioMaxHz;
    }

    const int bLo = analyzer.bandFromHz(lo);
    const int bHi = analyzer.bandFromHz(hi);
  if (bLo == prevBandLo[i] && bHi == prevBandHi[i] &&
      s.enabled == prevBandOn[i]) {
    continue; // unchanged, leave the pixels alone
  }
  prevBandLo[i] = bLo;
  prevBandHi[i] = bHi;
  prevBandOn[i] = s.enabled;

  const int y = bandRowY + (int)i * kBandRowPitch;
  M5.Display.fillRect(0, y, M5.Display.width(), 2, kBlack);
  const int x = bLo * bandWidth;
  const int w = (bHi - bLo + 1) * bandWidth - 1;
  // A switched-off light shows its window in grey.
  M5.Display.fillRect(x, y, w, 2, s.enabled ? ledColor(i) : kGrey);
}
}

// One row per light, showing what its output is actually doing.
void drawMeters() {
  for (size_t i = 0; i < kLedCount; i++) {
    const int target = (int)(leds[i].brightness() * (float)meterWidth + 0.5f);
    if (target > prevMeter[i]) {
      M5.Display.fillRect(kMeterX + prevMeter[i], meterY[i], target - prevMeter[i],
                          kMeterH, ledColor(i));
    } else if (target < prevMeter[i]) {
      M5.Display.fillRect(kMeterX + target, meterY[i], prevMeter[i] - target, kMeterH,
                          kBlack);
    }
    prevMeter[i] = target;
  }
}

void formatHz(char *out, size_t len, uint16_t hz) {
  if (hz >= 1000) {
    snprintf(out, len, "%.1fk", (double)hz / 1000.0);
  } else {
    snprintf(out, len, "%u", (unsigned)hz);
  }
}

// 'R' reactive, 'G' glow, '-' switched off.
char modeChar(size_t i) {
  if (!settings.led[i].enabled) {
    return '-';
  }
  return settings.led[i].mode == LedSettings::kGlow ? 'G' : 'R';
}

// A background colour is given to every glyph, so the text clears itself and the
// HUD can be refreshed without repainting the whole line.
void drawHud(float levelDb) {
  // Top line: one compact entry per light, e.g. "1:R150 2:R1.0k 3:R4.0k".
  char top[48];
  size_t n = 0;
  top[0] = '\0';
  for (size_t i = 0; i < kLedCount && n + 12 < sizeof(top); i++) {
    char hz[10];
    formatHz(hz, sizeof(hz), settings.led[i].centerHz);
    n += (size_t)snprintf(top + n, sizeof(top) - n, "%s%u:%c%s", i == 0 ? "" : " ",
                          (unsigned)i + 1, modeChar(i), hz);
  }

  // Bottom line: level, every light's brightness and the tempo,
  // e.g. "-38dB 12/45/78% 120bpm".
  char bottom[56];
  n = 0;
  n += (size_t)snprintf(bottom + n, sizeof(bottom) - n, "%+4.0fdB ", levelDb);
  for (size_t i = 0; i < kLedCount && n + 6 < sizeof(bottom); i++) {
    n += (size_t)snprintf(bottom + n, sizeof(bottom) - n, "%s%3.0f", i == 0 ? "" : "/",
                          leds[i].brightness() * 100.0f);
  }
  if (n + 2 < sizeof(bottom)) {
    n += (size_t)snprintf(bottom + n, sizeof(bottom) - n, "%%");
  }
  const uint16_t bpm = beat_.bpm();
  if (bpm > 0 && n + 9 < sizeof(bottom)) {
    snprintf(bottom + n, sizeof(bottom) - n, " %ubpm", (unsigned)bpm);
  }

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(kWhite, kBlack);
  M5.Display.setCursor(2, hudTopY);
  M5.Display.print(top);

  M5.Display.setTextColor(web.enabled() ? kApOn : kGrey, kBlack);
  M5.Display.setCursor(apLabelX, hudTopY);
  M5.Display.print("AP");

  M5.Display.setTextColor(kWhite, kBlack);
  M5.Display.setCursor(2, hudBottomY);
  M5.Display.print(bottom);
}

// Shown at boot (and when the access point is toggled), because the phone needs
// the network name and the password.
void showBanner() {
  const bool on = web.enabled();

  M5.Display.fillScreen(kBlack);
  M5.Display.setTextColor(kWhite, kBlack);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(4, 6);
  M5.Display.print(on ? "Connect to Wi-Fi:" : "Access point off");

  M5.Display.setTextColor(on ? kApOn : kGrey, kBlack);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(4, 18);
  M5.Display.print(on ? WebUi::kSsid : "---");

  M5.Display.setTextSize(1);
  M5.Display.setTextColor(kWhite, kBlack);
  if (on) {
    M5.Display.setCursor(4, 42);
    M5.Display.printf("password  %s", WebUi::kPassword);
    M5.Display.setCursor(4, 54);
    M5.Display.printf("open      http://%s", web.ip().c_str());
  } else {
    M5.Display.setCursor(4, 42);
    M5.Display.print("hold B to switch it");
    M5.Display.setCursor(4, 54);
    M5.Display.print("back on");
  }

  M5.Display.setTextColor(kGrey, kBlack);
  M5.Display.setCursor(4, 76);
  M5.Display.print("A mode L1   hold: L2");
  M5.Display.setCursor(4, 88);
  M5.Display.print("B mic gain  hold: Wi-Fi");
  M5.Display.setCursor(4, 100);
  char pins[32];
  size_t n = 0;
  pins[0] = '\0';
  for (size_t i = 0; i < kLedCount && n + 8 < sizeof(pins); i++) {
    n += (size_t)snprintf(pins + n, sizeof(pins) - n, "%sG%d", i == 0 ? "" : " ",
                          kLedPins[i]);
  }
  M5.Display.printf("lights on %s", pins);
}

void finishBanner() {
  M5.Display.fillScreen(kBlack);
  for (size_t b = 0; b < SpectrumAnalyzer::kBands; b++) {
    prevBar[b] = 0;
  }
  for (size_t i = 0; i < kLedCount; i++) {
    prevMeter[i] = 0;
    prevBandLo[i] = -1;
    prevBandHi[i] = -1;
    prevBandOn[i] = 0xFF;
  }
  lastHud = 0;
  lastFrameUs = 0; // don't count the banner pause as a frame interval
}

void toggleMode(size_t i) {
  settings.led[i].mode = settings.led[i].mode == LedSettings::kReactive
                             ? LedSettings::kGlow
                             : LedSettings::kReactive;
  noteSettingsChanged();
  lastHud = 0;
}

} // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);

  M5.Display.setRotation(1);
  M5.Power.setLed(0); // red LED shares GPIO19 with the IR emitter
  M5.Display.setBrightness(150);
  M5.Display.fillScreen(kBlack);

  bandWidth = M5.Display.width() / SpectrumAnalyzer::kBands;
  barWidth = bandWidth - 1;

  const int height = M5.Display.height();
  hudBottomY = height - 10;
  apLabelX = M5.Display.width() - 14;
  meterWidth = M5.Display.width() - kMeterX - 2;

  // Derived from the number of lights, so adding a channel needs no hand-tuned
  // pixel offsets.
  const int metersH = (int)kLedCount * (kMeterH + 2);
  const int bandRowsH = (int)kLedCount * kBandRowPitch;
  const int meterTop = hudBottomY - 2 - metersH;
  for (size_t i = 0; i < kLedCount; i++) {
    meterY[i] = meterTop + (int)i * (kMeterH + 2);
  }
  bandRowY = meterTop - 2 - bandRowsH;
  barBottom = bandRowY - 1;

  settings = defaultSettings();

  if (!store.begin()) {
    Serial.println("settings will not persist");
  } else if (store.loadLive(settings)) {
    clampSettings(settings);
    Serial.println("loaded saved settings");
  } else {
    Serial.println("no saved settings, using defaults");
  }

  for (size_t i = 0; i < kLedCount; i++) {
    leds[i].begin(kLedPins[i], kLedChannels[i]);
    pwmInit(i);
    pwmWrite(i, 0);
  }

  if (!analyzer.begin(kSampleRate)) {
    M5.Display.setTextColor(kWhite, kBlack);
    M5.Display.setCursor(4, 60);
    M5.Display.print("mic init failed");
    Serial.println("mic init failed");
  }
  analyzer.setGainDb(settings.gainDb);

  web.setup(&settings, &store);
  if (!web.begin()) {
    Serial.println("web ui failed to start");
  }
  showBanner();
  delay(2800);
  finishBanner();

  Serial.printf("ready: %u lights, fft %u/%u Hz, ap http://%s\n", (unsigned)kLedCount,
                (unsigned)SpectrumAnalyzer::kSamples, (unsigned)kSampleRate,
                web.ip().c_str());
  for (size_t i = 0; i < kLedCount; i++) {
    Serial.printf("  light %u: G%d, ledc channel %u\n", (unsigned)i + 1,
                  leds[i].pin(), (unsigned)leds[i].channel());
  }
}

void loop() {
  M5.update();
  web.update();

  if (web.consumeChanged()) {
    noteSettingsChanged();
  }
  if (settingsDirty && millis() - settingsChangedAt >= kSaveDelayMs) {
    settingsDirty = false;
    Serial.println(store.saveLive(settings) ? "[cfg] saved" : "[cfg] save failed");
  }

  if (M5.BtnA.wasClicked()) {
    toggleMode(0);
  }
  if (M5.BtnA.wasHold() && kLedCount > 1) {
    toggleMode(1);
  }

  if (M5.BtnB.wasClicked()) {
    settings.gainDb = (uint8_t)(settings.gainDb + 6);
    if (settings.gainDb > 18) {
      settings.gainDb = 0;
    }
    noteSettingsChanged();
  }

  if (M5.BtnB.wasHold()) {
    const bool on = web.setEnabled(!web.enabled());
    showBanner();
    delay(on ? 2000 : 800);
    finishBanner();
  }

  // The gain can be changed from the buttons or from the phone, so keep the
  // analyzer in sync in one place.
  if (syncedGainDb != settings.gainDb) {
    syncedGainDb = settings.gainDb;
    analyzer.setGainDb(syncedGainDb);
    lastHud = 0;
  }

  if (!analyzer.update()) {
    delay(5);
    return;
  }

  // The microphone paces the loop, so measure the real frame time instead of
  // assuming a fixed one: the glow period and the decay stay accurate.
  const uint32_t nowUs = micros();
  uint32_t dtMs = lastFrameUs == 0 ? 32 : (nowUs - lastFrameUs) / 1000;
  lastFrameUs = nowUs;
  if (dtMs == 0) {
    dtMs = 1;
  } else if (dtMs > 250) {
    dtMs = 250;
  }

  Telemetry tele;
  tele.levelPct = (uint8_t)(analyzer.level() * 100.0f + 0.5f);
  tele.dbfs = (int16_t)lroundf(analyzer.levelDb());
  tele.peakHz = (uint16_t)lroundf(analyzer.peakHz());

  const BeatInfo beat =
      beat_.update(analyzer.flux(), dtMs, settings.beatSensitivityPct);
  tele.bpm = beat_.bpm();
  tele.beatPct = (uint8_t)(beat.pulse * 100.0f + 0.5f);

  for (size_t i = 0; i < kLedCount; i++) {
    const LedSettings &s = settings.led[i];
    // Each light only watches its own window. Glow ignores the microphone, and a
    // switched-off light does not need a band either.
    const float level = (s.enabled && s.mode == LedSettings::kReactive)
                            ? analyzer.bandLevel((float)s.centerHz, (float)s.widthHz)
                            : 0.0f;
    const uint32_t duty = leds[i].update(s, level, beat, dtMs);
    pwmWrite(i, duty);

    tele.brightPct[i] = (uint8_t)(leds[i].brightness() * 100.0f + 0.5f);
    tele.duty[i] = (uint16_t)duty;
  }
  web.setTelemetry(tele);

  M5.Display.startWrite();
  drawSpectrum(analyzer.bands());
  drawBandMarkers();
  drawMeters();
  if (millis() - lastHud >= 100) {
    lastHud = millis();
    drawHud(analyzer.levelDb());
  }
  M5.Display.endWrite();

  if (millis() - lastStats >= 1000) {
    lastStats = millis();
    char out[64];
    size_t n = (size_t)snprintf(out, sizeof(out), "lvl %3.0f%% %+4.0f dBFS peak %5.0f Hz |",
                                analyzer.level() * 100.0f, analyzer.levelDb(),
                                analyzer.peakHz());
    for (size_t i = 0; i < kLedCount && n + 8 < sizeof(out); i++) {
      n += (size_t)snprintf(out + n, sizeof(out) - n, " %u:%.0f%%", (unsigned)i + 1,
                            leds[i].brightness() * 100.0f);
    }
    Serial.println(out);
  }
}
