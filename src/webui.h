#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "config.h"
#include "settings.h"
#include "store.h"

// Live values as shown on the phone. Kept as integers so the JSON never depends
// on float printf support.
struct Telemetry {
  uint8_t levelPct = 0; // incoming audio level, broadband
  int16_t dbfs = -99;
  uint16_t peakHz = 0;
  uint8_t brightPct[kLedCount] = {};
  uint16_t duty[kLedCount] = {};
  uint16_t bpm = 0;
  uint8_t beatPct = 0; // decaying beat pulse, for the page's beat indicator
};

// Access point + web page used as the remote control. The phone connects
// straight to the board, so no router is involved.
class WebUi {
public:
  static constexpr char kSsid[] = "SoundLight";
  static constexpr char kPassword[] = "soundlight";

  void setup(Settings *settings, SettingsStore *store);
  bool begin();
  void update() {
    if (enabled_) {
      server_.handleClient();
    }
  }

  // True once after a request changed the live settings, so the caller can
  // persist them (debounced, to spare the flash).
  bool consumeChanged() {
    const bool changed = changed_;
    changed_ = false;
    return changed;
  }

  // True if the user tapped this channel's drum disk since the last frame.
  bool consumeTap(size_t i) {
    if (i >= kLedCount || !pendingTap_[i]) {
      return false;
    }
    pendingTap_[i] = false;
    return true;
  }

  // Returns -1 if unchanged, 0 if released, 1 if pressed/held since last frame.
  int8_t consumeHold(size_t i) {
    if (i >= kLedCount) {
      return -1;
    }
    const int8_t v = pendingHold_[i];
    pendingHold_[i] = -1;
    return v;
  }

  bool enabled() const { return enabled_; }
  bool setEnabled(bool on);
  String ip() const;
  void setTelemetry(const Telemetry &t) { tele_ = t; }

private:
  void bindRoutes();
  String stateJson() const;
  void handleRoot();
  void handleState();
  void handleParams();
  void handleTap();
  void handlePreset();
  void handleReset();
  void applyArg(const char *name, uint8_t *dst);
  void applyArg(const char *name, uint16_t *dst);

  WebServer server_{80};
  Settings *settings_ = nullptr;
  SettingsStore *store_ = nullptr;
  Telemetry tele_;
  bool pendingTap_[kLedCount] = {};
  int8_t pendingHold_[kLedCount] = {-1, -1, -1};
  bool enabled_ = false;
  bool routesBound_ = false;
  bool changed_ = false;
};
