#include "led.h"

#include <math.h>

#include "config.h"

namespace {

constexpr float kTwoPi = 6.28318531f;

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// One step of an exponential approach towards target. A time constant of 0
// means "no memory at all", i.e. follow the input immediately.
inline float approach(float current, float target, uint32_t dtMs, uint16_t tauMs) {
  if (tauMs == 0) {
    return target;
  }
  const float alpha = 1.0f - expf(-(float)dtMs / (float)tauMs);
  return current + (target - current) * alpha;
}

} // namespace

void Led::begin(int pin, uint8_t channel) {
  pin_ = pin;
  channel_ = channel;
  smoothed_ = 0.0f;
  brightness_ = 0.0f;
  phase_ = 0.0f;
  duty_ = 0;
  lastMode_ = 0xFF;
  echoLeft_ = 0;
  echoNextMs_ = 0;
  echoAmp_ = 0.0f;
  sinceOnsetMs_ = 1000000u;
  onsetRef_ = 0.0f;
  armed_ = true;
}

uint32_t Led::update(const LedSettings &s, float level, const BeatInfo &beat,
                     uint32_t dtMs) {
  if (dtMs == 0) {
    dtMs = 1;
  }

  // Restart each mode from a clean state, so switching never jumps.
  if (s.mode != lastMode_) {
    lastMode_ = s.mode;
    phase_ = 0.0f;    // glow starts at the dim end of the cycle
    smoothed_ = 0.0f; // reactive ramps up from dark
    echoLeft_ = 0;
    echoNextMs_ = 0;
    echoAmp_ = 0.0f;
    sinceOnsetMs_ = 1000000u;
    onsetRef_ = 0.0f;
    armed_ = true;
  }

  float b;
  if (s.mode == LedSettings::kGlow) {
    // Glow can follow the tempo instead of a fixed period, and restart its cycle
    // on the beat so the brightest point lands on it.
    uint16_t periodMs = s.glowPeriodMs;
    if (s.beatMode == LedSettings::kBeatGlowSync && beat.periodMs > 0) {
      periodMs = beat.periodMs;
      if (beat.onBeat) {
        phase_ = 0.5f; // a raised cosine peaks at 0.5
      }
    }
    const uint16_t lo = Settings::kMinGlowPeriodMs;
    const uint16_t hi = Settings::kMaxGlowPeriodMs;
    if (periodMs < lo) {
      periodMs = lo;
    } else if (periodMs > hi) {
      periodMs = hi;
    }

    phase_ += (float)dtMs / (float)periodMs;
    if (phase_ >= 1.0f) {
      phase_ -= floorf(phase_);
    }
    // Raised cosine: no corner at the top or bottom of the cycle.
    const float wave = 0.5f - 0.5f * cosf(kTwoPi * phase_);
    const float loPct = (float)s.glowMinPct / 100.0f;
    const float hiPct = (float)s.glowMaxPct / 100.0f;
    b = loPct + (hiPct - loPct) * wave;
  } else {
    const float gate = (float)s.gatePct / 100.0f;
    const float inLevel = clamp01(level);
    // Level as it will appear after the gate, from the microphone only: the
    // synthetic echoes must never retrigger the detector.
    const float inGated =
        inLevel <= gate ? 0.0f : (inLevel - gate) / (1.0f - gate);

    // Saturating, so the refractory check cannot wrap after a long run.
    sinceOnsetMs_ = sinceOnsetMs_ > 1000000u ? 1000000u : sinceOnsetMs_ + dtMs;

    smoothed_ = approach(smoothed_, inLevel, dtMs,
                         inLevel > smoothed_ ? s.attackMs : s.releaseMs);

    // Replay the trigger: each repeat re-injects its amplitude, so the normal
    // release shape carries it and no extra envelope is needed. Played before
    // the detector so a freshly scheduled echo gets its full first interval.
    if (echoLeft_ > 0) {
      const uint32_t step = echoNextMs_ > dtMs ? dtMs : echoNextMs_;
      echoNextMs_ -= step;
      if (echoNextMs_ == 0) {
        // echoAmp_ is a post-gate brightness, so convert it back to the
        // pre-gate level the gate expects.
        const float injected = gate + echoAmp_ * (1.0f - gate);
        if (injected > smoothed_) {
          smoothed_ = injected;
        }
        echoAmp_ *= (float)s.echoFeedbackPct / 100.0f;
        echoLeft_--;
        echoNextMs_ = s.echoSpacingMs;
      }
    }

    // Onset detection: fire when the sound rises well above the level the
    // detector last settled at, and re-arm only once it has fallen back.
    constexpr float kOnsetFloor = 0.05f;
    constexpr float kOnsetRise = 1.6f;
    if (s.echoCount > 0 && armed_ && inGated > kOnsetFloor &&
        inGated > onsetRef_ * kOnsetRise &&
        sinceOnsetMs_ >= s.echoSpacingMs) {
      armed_ = false;
      sinceOnsetMs_ = 0;
      onsetRef_ = inGated;
      echoLeft_ = s.echoCount;
      echoAmp_ = inGated * (float)s.echoFeedbackPct / 100.0f;
      echoNextMs_ = s.echoSpacingMs;
    } else if (inGated > onsetRef_) {
      onsetRef_ = inGated; // follow the envelope upwards
    }
    if (inGated < onsetRef_ * 0.5f || inGated <= kOnsetFloor) {
      armed_ = true; // the sound fell back, so the next rise is a new onset
      onsetRef_ = inGated;
    }

    // The gate keeps the light dark in a quiet room and rescales the rest, so
    // a soft passage still uses the whole output range.
    b = smoothed_ <= gate ? 0.0f : (smoothed_ - gate) / (1.0f - gate);

    // Every beat adds a flash on top of whatever the music is doing.
    if (s.beatMode == LedSettings::kBeatBoost && beat.pulse > 0.0f) {
      b += ((float)s.beatBoostPct / 100.0f) * beat.pulse;
    }
  }

  brightness_ = clamp01(b);
  // The model keeps running while the light is switched off (phase and
  // smoothing keep tracking), so switching it back on resumes in sync rather
  // than jumping from a stale value.
  if (!s.enabled) {
    brightness_ = 0.0f;
  }
  duty_ = (uint32_t)(powf(brightness_, kLedGamma) * (float)kLedPwmMax + 0.5f);
  return duty_;
}
