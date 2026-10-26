# Internals

[← back to the README](../README.md)

## Pipeline and timing

```
mic (PDM, 16 kHz)
   → 512-sample frame, DC removed                     ~32 ms
   → FFT (Hann window) — ESP-DSP or arduinoFFT
   → magnitudes + spectral flux
        ├→ band energy per channel → Led → PWM
        └→ beat detector → pulse + BPM
   → 30 log bands → LCD
```

The microphone paces the loop: each `update()` blocks for one frame (~32 ms), so the
screen refreshes at ~30 fps and the web page polls at 400 ms. `handleClient()` runs
once per frame — plenty for a phone.

The analyser reports **unsmoothed** values on purpose. All time-based shaping
(attack, release, echo, glow, beat pulse) lives in the `Led` objects, which work from
the real elapsed time between frames. That is why a "200 ms release" really is
200 ms no matter how the loop is paced.

## FFT backends

The FFT sits behind [src/fft_backend.h](../src/fft_backend.h), with two
implementations selected at compile time (see
[Build and flash](build-and-flash.md#fft-backends) for how to choose).

They are **numerically interchangeable** — on the same signals they agree to within
0.0005 on the 0..1 band scale, so switching does not change how the lights behave:

| Test signal | ESP-DSP | arduinoFFT |
|---|---|---|
| 1 kHz at 0.05 FS | 0.7017 | 0.7020 |
| 2 kHz at 0.10 FS | 0.8644 | 0.8647 |
| 300 Hz at 0.02 FS | 0.4865 | 0.4868 |

That needed one subtlety. `arduinoFFT`'s "Hann" window is actually
`0.54·(1−cos)`, not the textbook `0.5·(1−cos)`. The two are the same shape scaled by
1.08, so arduinoFFT's magnitudes come out 1.08× larger and its Parseval divisor is
3.49 rather than 3.0. Each backend reports its own divisor, which keeps the
calibration identical from the outside.

The ESP-DSP path is worth reading with the header open: `dsps_fft2r_fc32` expects
interleaved complex data and leaves the bins in **bit-reversed** order, so the code
packs real samples with the window applied, transforms, then calls
`dsps_bit_rev_fc32` before taking magnitudes.

## Project layout

| File | Role |
|------|------|
| [src/config.h](../src/config.h) | **All hardware config**: pins, channels, PWM, sample rate, FFT size |
| [src/settings.h](../src/settings.h) | Parameter structs and ranges; clamping lives here |
| [src/settings_blob.*](../src/settings_blob.h) | Versioned binary encoding for storage |
| [src/store.*](../src/store.h) | NVS persistence: live settings + 2 presets |
| [src/audio_fft.*](../src/audio_fft.h) | Capture, FFT, bands, flux, band energy |
| [src/fft_backend*.cpp](../src/fft_backend.h) | The two FFT implementations |
| [src/beat.*](../src/beat.h) | Onset detection and tempo tracking |
| [src/led.*](../src/led.h) | One channel: attack / release / gate / echo / glow / beat |
| [src/webui.*](../src/webui.h) | Access point, HTTP API, embedded HTML |
| [src/main.cpp](../src/main.cpp) | Wiring of the above, buttons, display |

Only the PWM write lives in `main.cpp`; `Led` has no Arduino dependency, so the
whole temporal model runs on a workstation. That is how the numbers in
[Accuracy](#accuracy) were measured.

## Storage

NVS namespace `sndlight`, keys `live`, `preset0`, `preset1`, each a **69-byte**
versioned blob (3-byte header + 22 bytes per channel).

* A wrong length or unknown version is **rejected** and defaults are used, so
  upgrading firmware cannot misread an old layout (it does discard saved presets).
* Values read back are clamped, so a corrupted entry cannot produce an
  out-of-range configuration.
* Writes are debounced by 800 ms to spare the flash.

## Customising

### Hardware

[src/config.h](../src/config.h) holds everything wiring-dependent:

| Constant | Default | Meaning |
|----------|---------|---------|
| `kLedPins` | `{32, 33, 26}` | One PWM pin per channel (R / G / B) |
| `kLedChannels` | `{0, 1, 2}` | LEDC channel per channel |
| `kLedPwmFreq` | 5000 | PWM frequency (Hz) |
| `kLedPwmBits` | 10 | Resolution, so duty is 0..1023 |
| `kSampleRate` | 16000 | Microphone sample rate |
| `kFftSamples` | 512 | FFT length, power of two |
| `kAudioMinHz` / `kAudioMaxHz` | 65 / 7600 | Analysis range, also the band limits |
| `kLedGamma` | 1.6 | Perceptual correction of the PWM duty |

### Analysis

Behaviour constants live in [src/audio_fft.cpp](../src/audio_fft.cpp):

| Constant | Meaning |
|----------|---------|
| `kBandFloorDb`, `kBandRangeDb` | dBFS window mapped onto the bar height |
| `kLevelFloorDb`, `kLevelFullDb` | dBFS window mapped onto the 0..1 control level |

For a finer low end, raise `kFftSamples` to 1024 (15.6 Hz bins, ~64 ms frames); for
a faster response, go the other way. It must stay a power of two.

The beat detector's onset sensitivity (rise factor and floor) is in
[src/beat.cpp](../src/beat.cpp); the user-facing knob is the sensitivity slider.

### Adding a fourth channel

Add one entry to `kLedPins` and one to `kLedChannels`. The build asserts that the
arrays match, and the screen layout, the JSON, the web page and the storage all size
themselves from `kLedCount` — no other edits needed.

## Accuracy

Measured on synthetic signals with the host test suite, not on a stage:

| Claim | Measured |
|-------|----------|
| Beat detection, 120 BPM clicks | 117 BPM reported, 499.5 ms intervals |
| Beat detection, 90 BPM clicks | 89 BPM, 665.6 ms |
| Beat detection, 140 BPM clicks | 144 BPM, 427.9 ms |
| False beats in silence | none over 200 frames |
| Sustained tone | one trigger, not a stream |
| Echo timing | exactly 100 / 200 / 300 ms at 100 ms spacing |
| Echo amplitudes | 0.600 / 0.360 / 0.216 at 60 % feedback |
| Band calibration | full-width window matches broadband within ±0.0005 |

Honest caveats:

* **Click tracks flatter any beat detector.** Expect more false onsets on dense
  material (drum and bass, dense percussion). Sensitivity is the knob.
* **Tempo is accurate to about ±3 BPM**, limited by the 32 ms analysis grid. Fine for
  flashing; glow-sync self-corrects because the phase resets on every beat.
* **Tempo is folded into 60–170 BPM**, because a missed beat is indistinguishable
  from a tempo half as fast. A 60 BPM track may read 120.
* **ESP-DSP was verified to compile and link for the target, and its maths verified
  on a workstation** through a portable reference implementation. It has not been run
  on hardware in this repository's tests.
