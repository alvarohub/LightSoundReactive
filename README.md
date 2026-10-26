# Light Show — Sound and Meaning

A sound-reactive light show on the **M5StickC Plus2**, an ESP32 board the size of a
lighter. Three independent PWM channels drive an **R / G / B** light, each listening
to its **own frequency band** (bass on red, mids on green, highs on blue), with
beat detection and a phone interface served from the board's own Wi-Fi access point.

No app to install, no router needed: connect to the board's network and open a web
page.

```
                       +-> 30 log bands (65 Hz-7.6 kHz) -> LCD spectrum
  PDM mic -> FFT 512 @ 16 kHz
                       +-> spectral flux -> beat detection -> BPM
                       +-> per-channel band energy (center +/- width/2)
                                     |
                    +----------------+----------------+
                    |                                 |
              Reactive mode                        Glow mode
        attack / release / gate / echo      breathing, optionally
                                             synced to the beat
                    |                                 |
              PWM on G32, G33, G26  ->  R / G / B
```

**Status:** a hobby project that works, shared because people asked. It runs at
~31 fps, uses 19 % of the RAM, and the audio path is honest about its limits (see
[Accuracy](#accuracy)). Written for one specific board and three LEDs; the ideas
that would make it bigger are in [IDEAS.md](IDEAS.md).

---

## Contents

- [What you need](#what-you-need)
- [Quick start](#quick-start)
- [Wiring](#wiring)
- [Using it](#using-it)
- [Modes and parameters](#modes-and-parameters)
- [HTTP API](#http-api)
- [How it works](#how-it-works)
- [Customising](#customising)
- [Troubleshooting](#troubleshooting)
- [Accuracy](#accuracy)
- [Licence](#licence)
- [Credits](#credits)

---

## What you need

### Hardware

| Part | Notes |
|------|-------|
| **M5StickC Plus2** (SKU K016-P2) | ESP32-PICO-V3-02, 8 MB flash, 2 MB PSRAM, 200 mAh battery, built-in microphone |
| 3 × LEDs | Or one RGB LED module; or a strip later via MOSFETs |
| 3 × resistors, 220–330 Ω | One per LED |
| 1 × Grove cable | For the first two channels |
| Wire for the third channel | It goes to the 8-pin header, not the Grove port |
| *Optional:* 3 × logic-level MOSFETs | For strips or anything above ~12 mA |

The built-in **SPM1423 microphone** is what makes this possible without extra
parts — no external mic, no I2S wiring.

### Software

- [PlatformIO](https://platformio.org) — either the VS Code extension or the CLI
  (`pip install platformio`). The Arduino framework and all libraries are
  downloaded automatically on the first build.

No separate Arduino IDE setup is needed.

---

## Quick start

### 1. Get the code

```sh
git clone https://github.com/alvarohub/LightShow_SoundAndMeaning.git
cd LightShow_SoundAndMeaning
```

### 2. Build and flash

```sh
pio run                 # build
pio run -t upload       # flash over USB-C
pio device monitor      # 115200 baud, watch it talk
```

The first build downloads the ESP32 toolchain (~200 MB), so it takes a few
minutes. Later builds take seconds.

### 3. Connect your phone

The board starts its own Wi-Fi network on boot:

| | |
|---|---|
| Network | `SoundLight` |
| Password | `soundlight` |
| Page | <http://192.168.4.1> |

The same details appear on the LCD for a few seconds after boot, so you do not
need to remember them.

> There is no internet on that network — it is the board talking to you directly.
> Some phones silently fall back to mobile data; if the page does not load, turn
> mobile data off for a moment.

### 4. Check it works

The screen should show a live spectrum and three brightness meters, and the LEDs
should react when you play music near the board. If nothing lights up, press
**button A** (mode) or check [Troubleshooting](#troubleshooting).

---

## Wiring

Two channels are on the **Grove connector**, the third is on the **8-pin HAT
header**:

| Channel | Pin | Where |
|---------|-----|-------|
| **R** | **G32** | Grove port, yellow wire |
| **G** | **G33** | Grove port, white wire |
| **B** | **G26** | 8-pin HAT header |

M5Stack's pinmap for this board gives the HY2.0-4P port as `GND / 5V / G32 / G33`,
and lists `G0, G25, G26, G36` as the header pins — so **G26 is not reachable on
the Grove connector**. Check the header silkscreen for the exact pin order on your
unit.

All three pins are output-capable and reach the LED controller (LEDC) through the
GPIO matrix. **Never** power LEDs from the Grove 5 V pin.

### Prototype with plain LEDs

```
G32 ----[ 220R .. 330R ]----|>|---- GND        (R, Grove yellow)
G33 ----[ 220R .. 330R ]----|>|---- GND        (G, Grove white)
G26 ----[ 220R .. 330R ]----|>|---- GND        (B, HAT header)
GND  ---- Grove pin 1 (black) or any GND
```

Keep each GPIO below ~12 mA — fine for three indicator LEDs, not for a strip.

### Driving real lights with MOSFETs

```
G26 --[100R]-- Gate
                |
             [10k]  gate pulldown to GND
                |
GND ------------+---- Source
                   N-channel logic-level MOSFET (AO3400, IRLZ44N, ...)
                      Drain ---- strip cathode (-)
                      strip anode (+) ---- external supply +
                      external supply ------- GND (shared with the StickC)
```

Repeat for G32 and G33. Use a **logic-level** device (it must switch fully at
Vgs = 3.3 V). Each strip needs its own supply with a **common ground**; never the
board's 5 V pin. 5 kHz PWM suits MOSFETs — for very long or capacitive loads,
lower `kLedPwmFreq` to 1–2 kHz in [src/config.h](src/config.h).

---

## Using it

### The phone interface

Open <http://192.168.4.1> and pick a channel with the **R / G / B** buttons:

| Signal | Meaning |
|--------|---------|
| **Thick blue ring** | The channel you are editing |
| **Green fill** | That channel is powered on |
| **Red fill** | That channel is powered off |

The panel below repeats it in words — *Channel R — powered on · reactive · 150 Hz* —
so the ring is never the only clue. Below that are three output meters, one per
channel, in their own colour, plus the live level, dBFS, peak frequency and BPM.

Everything applies immediately. Changes are saved automatically after 800 ms of
inactivity (debounced, to spare the flash).

### Buttons

| Input | Action |
|-------|--------|
| **A** | Toggle R between reactive and glow |
| **Hold A** | Same for G |
| **B** | Cycle microphone gain (0 / 6 / 12 / 18 dB) |
| **Hold B** | Toggle the access point |

There are only two usable buttons, so **B** (the third channel) is controlled from
the phone, as is every channel's power switch.

> The access point is by far the biggest power draw. On the 200 mAh battery expect
> roughly an hour untethered; hold **B** to switch Wi-Fi off when you do not need
> the remote.

### The screen (240 × 135, landscape)

```
1:R150 2:R1.0k 3:R4.0k               AP     <- mode per channel + window, Wi-Fi
|  | | | |  | | | |  |  |  |  |  |  |  |   30 spectrum bars
===========                            <- where each channel listens
--   --      ----------                     (one thin marker per channel)
[=======        ]  R out
[=====          ]  G out                <- output meters, one per channel
[               ]  B out
-38dB  61/34/ 0% 120bpm                <- level, brightness, tempo
```

A channel shows `R` (reactive), `G` (glow) or `-` (switched off). The serial port
prints the same values once per second.

### Presets

* **Save 1 / Save 2** store the whole configuration — all three channels, every
  parameter.
* **Load 1 / Load 2** bring it back. A slot that was never saved is greyed out.
* **Factory reset** restores the defaults: R on the bass, G on the mids, B on the
  highs. It deliberately **keeps your presets**.

---

## Modes and parameters

Each channel has its own complete set. All values are live.

### Reactive (sound → light)

| Parameter | Range | Default | What it does |
|-----------|-------|---------|--------------|
| Center frequency | 65–7600 Hz | R 150, G 1000, B 4000 | Middle of the band this channel listens to |
| Band width | 40–16000 Hz | R 250, G 1400, B 5000 | Total width, so it spans center ± width/2 |
| Attack | 0–1000 ms | 30 | How fast the light rises. `0` = instant |
| Release / decay | 0–2000 ms | 200 | Exponential decay when the sound drops — the "memory" |
| Noise gate | 0–50 % | 4 | Level below which the light stays dark |
| Echo repeats | 0–10 | 0 | How many times each trigger is replayed |
| Echo spacing | 20–500 ms | 120 | Time between repeats |
| Echo decay | 0–90 % | 60 | Each repeat keeps this share of the previous |

The band limits what the channel *hears*: center 1000 Hz with width 500 Hz reacts
to 750–1250 Hz only, so a kick at 60 Hz does nothing to it. The page shows the
effective window ("listens to 750 - 1250 Hz") while you drag.

A band wide enough to cover the whole spectrum behaves as a **broadband**
reaction — no separate switch. The band measurement is calibrated so a full-width
window reads the same as the broadband level, which is why the same gate and
attack work whether a channel is narrow or wide.

Both times are **time constants**: with release at 200 ms the light falls to 37 %
of its excursion after 200 ms, and 13 % after 400 ms. Setting either to 0 removes
the memory entirely.

**Echo** replays each trigger like a delay with feedback: 3 repeats, 120 ms
spacing and 60 % decay gives a flash at 100 %, then echoes at 60 %, 36 % and 22 %.
The repeats reuse the normal release shape, so they look like the original flash
rather than a separate effect. The trigger is an *onset* — the sound must rise
well above where the detector settled, then fall back — so a sustained tone fires
once instead of machine-gunning, and the echoes cannot retrigger it.

### Glow (breathing)

| Parameter | Range | Default | What it does |
|-----------|-------|---------|--------------|
| Period | 100–5000 ms | 2000 | One full dim → bright → dim cycle |
| Dim level | 0–100 % | 0 | Bottom of the cycle |
| Bright level | 0–100 % | 100 | Top of the cycle |

The sweep is a raised cosine, so it eases in and out instead of hitting a corner
at each end. Glow ignores the microphone and the frequency band.

### Beat detection

| Setting | Range | Default | Scope |
|---------|-------|---------|-------|
| Beat detection | Off / Flash / Sync glow | Off | Per channel |
| Beat flash strength | 0–100 % | 60 | Per channel |
| Beat sensitivity | 0–100 % | 50 | Shared |

* **Flash** — each beat adds a quick flash on top of the reactive level, so the
  channel still follows its band but punches on the beat.
* **Sync glow** — the glow period follows the detected tempo and the cycle
  restarts on every beat, so the brightest point lands on the beat instead of
  drifting.

Only one detector runs, because there is one microphone: sensitivity is shared,
but each channel decides what to do with the result.

### Microphone gain

`0` / `6` / `12` / `18` dB, applied before the analysis and shared by all
channels. Raise it if the bars sit flat in a quiet room.

---

## HTTP API

The web page is just a client of these endpoints, so anything the page can do,
your own scripts can do too. Plain HTTP on port 80, no authentication (it is a
local access point).

### Endpoints

| Method | Path | Purpose |
|--------|------|---------|
| `GET` | `/` | The control page |
| `GET` | `/api/state` | Full state as JSON (parameters + live values) |
| `POST` | `/api/params` | Change parameters, given in the query string |
| `POST` | `/api/preset?slot=1\|2&action=save\|load` | Presets |
| `POST` | `/api/reset` | Restore defaults (presets kept) |

`/api/params` takes any number of parameters in one request. Channel parameters
need a `led=` index; global parameters do not.

### Per-channel parameters (`led=0` R, `1` G, `2` B)

| Key | Range | Default |
|-----|-------|---------|
| `mode` | `0` reactive, `1` glow | `0` |
| `enabled` | `0` off, `1` on | `1` |
| `centerHz` | 65–7600 | 150 / 1000 / 4000 |
| `widthHz` | 40–16000 | 250 / 1400 / 5000 |
| `attackMs` | 0–1000 | 30 |
| `releaseMs` | 0–2000 | 200 |
| `gatePct` | 0–50 | 4 |
| `echoCount` | 0–10 | 0 |
| `echoSpacingMs` | 20–500 | 120 |
| `echoFeedbackPct` | 0–90 | 60 |
| `beatMode` | `0` off, `1` flash, `2` sync glow | `0` |
| `beatBoostPct` | 0–100 | 60 |
| `glowPeriodMs` | 100–5000 | 2000 |
| `glowMinPct` | 0–100 | 0 |
| `glowMaxPct` | 0–100 | 100 |

### Global parameters

| Key | Range | Default |
|-----|-------|---------|
| `gainDb` | `0` / `6` / `12` / `18` (snapped) | 0 |
| `beatSensitivityPct` | 0–100 | 50 |

### State response

```json
{
  "gainDb": 0,
  "beatSensitivityPct": 50,
  "leds": [
    {
      "mode": 0, "enabled": 1,
      "centerHz": 150, "widthHz": 250,
      "attackMs": 30, "releaseMs": 200, "gatePct": 4,
      "echoCount": 0, "echoSpacingMs": 120, "echoFeedbackPct": 60,
      "beatMode": 0, "beatBoostPct": 60,
      "glowPeriodMs": 2000, "glowMinPct": 0, "glowMaxPct": 100,
      "brightPct": 61, "duty": 637          <- live values
    }
  ],
  "levelPct": 42, "dbfs": -38, "peakHz": 440,
  "bpm": 120, "beatPct": 35,
  "hasPreset0": 1, "hasPreset1": 0
}
```

`brightPct`/`duty` are what the channel is outputting right now; `bpm` is 0 until
the detector has heard a few beats.

### Examples

```sh
# what is it doing right now?
curl http://192.168.4.1/api/state

# give red the bass, 250 Hz wide
curl -X POST 'http://192.168.4.1/api/params?led=0&centerHz=150&widthHz=250'

# make green glow slowly instead
curl -X POST 'http://192.168.4.1/api/params?led=1&mode=1&glowPeriodMs=3000'

# punch blue on every beat
curl -X POST 'http://192.168.4.1/api/params?led=2&beatMode=1&beatBoostPct=80'

# switch a channel off and back on
curl -X POST 'http://192.168.4.1/api/params?led=0&enabled=0'
curl -X POST 'http://192.168.4.1/api/params?led=0&enabled=1'

# presets and reset
curl -X POST 'http://192.168.4.1/api/preset?slot=1&action=save'
curl -X POST 'http://192.168.4.1/api/reset'
```

### Behaviour

* Unknown parameters are **ignored**; out-of-range values are **clamped** — a bad
  request can never reach the PWM stage.
* Every call returns the full state, so one round trip confirms the change.
* A preset operation that cannot be honoured answers `400` with the unchanged
  state (for example `action=load` on an empty slot).

---

## How it works

### Pipeline and timing

```
mic (PDM, 16 kHz)
   -> 512-sample frame, DC removed                     ~32 ms
   -> FFT (Hann window)
   -> magnitudes + spectral flux
        +-> band energy per channel -> Led -> PWM
        +-> beat detector -> pulse + BPM
   -> 30 log bands -> LCD
```

The microphone paces the loop: each `update()` blocks for one frame (~32 ms), so
the screen refreshes at ~30 fps and the web page polls at 400 ms. `handleClient()`
runs once per frame — plenty for a phone.

The analyser reports **unsmoothed** values on purpose. All time-based shaping
(attack, release, echo, glow, beat pulse) lives in the `Led` objects, which work
from the real elapsed time between frames. That is why a "200 ms release" is
really 200 ms no matter how the loop is paced.

### FFT backends

The FFT is behind a small interface in [src/fft_backend.h](src/fft_backend.h) with
two interchangeable implementations:

| Environment | Backend | Licence | RAM | Flash |
|---|---|---|---|---|
| `m5stick-c-plus2` (default) | **ESP-DSP** | Apache-2.0 | 18.7 % | 31.5 % |
| `m5stick-c-plus2-arduinodft` | arduinoFFT | GPL-3.0 | 17.5 % | 31.3 % |

```sh
pio run -e m5stick-c-plus2-arduinodft -t upload    # the arduinoFFT variant
```

ESP-DSP ships inside the Arduino-ESP32 core (no extra dependency) and dispatches
to its hand-written assembly routine on ESP32, so it is the faster of the two. It
costs ~4 KB more RAM for a scratch buffer and needs no extra download.

The two are **numerically interchangeable** — on the same test signals they agree
to within 0.0005 on the 0..1 band scale, so switching does not change how the
lights behave. That needed one subtlety: `arduinoFFT`'s "Hann" window is actually
`0.54·(1−cos)`, not the textbook `0.5·(1−cos)`, so its magnitudes come out 1.08×
larger and the Parseval divisor is 3.49 rather than 3.0. Each backend reports its
own divisor, which keeps the calibrations identical.

### Project layout

| File | Role |
|------|------|
| [src/config.h](src/config.h) | **All hardware config**: pins, channels, PWM, sample rate, FFT size |
| [src/settings.h](src/settings.h) | Parameter structs and their ranges (clamping lives here) |
| [src/settings_blob.*](src/settings_blob.h) | Versioned binary encoding for storage |
| [src/store.*](src/store.h) | NVS persistence: live settings + 2 presets |
| [src/audio_fft.*](src/audio_fft.h) | Capture, FFT, bands, flux, band energy |
| [src/fft_backend*.cpp](src/fft_backend.h) | The two FFT implementations |
| [src/beat.*](src/beat.h) | Onset detection and tempo tracking |
| [src/led.*](src/led.h) | One channel: attack/release/gate/echo/glow/beat |
| [src/webui.*](src/webui.h) | Access point, HTTP API, embedded HTML |
| [src/main.cpp](src/main.cpp) | Wiring of the above, buttons, display |

The PWM write is the only thing in `main.cpp`; `Led` has no Arduino dependency, so
the whole temporal model is testable on a workstation (which is how the numbers in
[Accuracy](#accuracy) were measured).

### Storage

Namespace `sndlight`, keys `live`, `preset0`, `preset1`, each a 69-byte versioned
blob (3-byte header + 22 bytes per channel). A wrong length or unknown version is
**rejected** and the defaults are used, and values read back are clamped — a
corrupted entry cannot produce an out-of-range configuration. Blobs from older
firmware versions are rejected rather than misread, so upgrading gives you the new
defaults (and discards saved presets).

---

## Customising

### Hardware

[src/config.h](src/config.h) holds everything wiring-dependent:

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

> LEDC channels 6 and 7 are already taken by M5Unified (the red status LED / IR
> emitter on G19, and the LCD backlight). Channels 0–2 stay clear of those.
>
> One caveat: M5Unified claims G25 and G26 for the external speaker HAT if you
> enable `cfg.external_speaker.hat_spk2`. It is off by default — leave it off while
> blue is wired to G26.

### Analysis

Behaviour constants live in [src/audio_fft.cpp](src/audio_fft.cpp):

| Constant | Meaning |
|----------|---------|
| `kBandFloorDb`, `kBandRangeDb` | dBFS window mapped onto the bar height |
| `kLevelFloorDb`, `kLevelFullDb` | dBFS window mapped onto the 0..1 control level |

For a finer low end, raise `kFftSamples` to 1024 (15.6 Hz bins, ~64 ms frames);
for a faster response, go the other way. It must stay a power of two.

The beat detector's onset sensitivity (rise factor and floor) is in
[src/beat.cpp](src/beat.cpp); the *user-facing* knob is the sensitivity slider.

### Adding a fourth channel

Add one entry to `kLedPins` and one to `kLedChannels`. The build asserts that the
arrays match, and the screen layout, the JSON, the web page and the storage all
size themselves from `kLedCount` — no other edits needed.

---

## Troubleshooting

| Symptom | Likely cause / fix |
|---------|--------------------|
| Port not recognised over a USB-C→C cable | Known on this board. Unplug, power off (long-press power until the green LED lights), then reconnect. |
| Upload fails, "port busy" | Close `pio device monitor` first; only one program can hold the port. |
| Upload fails, board not responding | Hold the power button while starting the upload. |
| No `SoundLight` network appears | Hold **B** to enable the access point, and check the serial output for `[web] ... -> http://192.168.4.1`. |
| Page will not load | Some phones fall back to mobile data on a network without internet; disable mobile data, or set the Wi-Fi connection to "stay connected". |
| LEDs never light | Check you are on the right pin (B is on the header, not Grove), LED polarity, that the channel is **on**, and that the gate is not too high. |
| One channel looks dead in reactive mode | Its band may be narrow and the music may have nothing there. Widen `widthHz` to test. |
| Reacts to nothing / almost nothing | Raise the microphone gain (button **B**) or lower the gate. |
| Beat flashes constantly on a busy track | Lower **Beat sensitivity**, or raise the noise gate. |
| BPM reads `--` | Needs a few clear beats; sparse or rubato material will not produce a tempo. |
| LED flickers or buzzes | Long wires or a capacitive load: lower `kLedPwmFreq` to 1–2 kHz. |
| Settings forgotten after reboot | NVS write needs 800 ms of quiet after the last change — wait a second before pulling power. |
| Screen stays on the Wi-Fi banner | Normal for ~3 s at boot; it clears itself. |

---

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
| Band calibration | full-width window matches broadband within ±0.0005 (both backends) |
| Backend agreement | ESP-DSP vs arduinoFFT within 0.0005 on the 0..1 scale |

Honest caveats:

* **Click tracks flatter any beat detector.** Expect more false onsets on dense
  material (drum and bass, dense percussion). Sensitivity is the knob.
* **Tempo is accurate to about ±3 BPM**, limited by the 32 ms analysis grid. Fine
  for flashing; glow-sync self-corrects because the phase resets on every beat.
* **The tempo is folded into 60–170 BPM**, because a missed beat is
  indistinguishable from a tempo half as fast. A 60 BPM track may read 120.
* **ESP-DSP was verified to compile and link for the target, and its maths verified
  on a workstation** through a portable reference implementation. It has not been
  run on hardware in this repository's tests.

---

## Licence

**MIT** — see [LICENSE](LICENSE). Use it, change it, sell it, just keep the
copyright notice.

Third-party components:

| Component | Licence | Notes |
|-----------|---------|-------|
| M5Unified, M5GFX | MIT | M5Stack / lovyan03 |
| ESP-DSP | Apache-2.0 | Ships with Arduino-ESP32; used by the default build |
| arduinoFFT | **GPL-3.0** | Only downloaded by the `…-arduinodft` environment |

Because arduinoFFT is GPL-3.0, a firmware binary built from the
`m5stick-c-plus2-arduinodft` environment is itself **GPL-3.0**. The default
environment (`m5stick-c-plus2`, ESP-DSP) contains only MIT and Apache-2.0 code, so
its binaries carry no copyleft obligation beyond attribution.

Why GPL is worth flagging: it is unusual in the Arduino ecosystem, where MIT and
BSD dominate precisely so people can copy snippets freely.

## Credits

- [M5Unified / M5GFX](https://github.com/m5stack/M5Unified) — board support, display,
  microphone. The PDM capture that makes all of this possible is theirs.
- [ESP-DSP](https://github.com/espressif/esp-dsp) — the optimised FFT.
- [arduinoFFT](https://github.com/kosme/arduinoFFT) — the optional backend.
- Beat detection follows the classic spectral-flux + adaptive-threshold recipe that
  runs through most real-time onset detectors; the tempo estimate is the median
  inter-onset interval.

Ideas for what comes next — voice activity detection, a semantic "AIqualizer", OSC
control, a manual colour pad — are in [IDEAS.md](IDEAS.md).
