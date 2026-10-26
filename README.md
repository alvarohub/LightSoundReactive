# LightSoundReactive

Three sound-reactive channels — **R, G, B** — on the **M5StickC Plus2**
(ESP32-PICO-V3-02), each with its own frequency window, plus a phone-friendly web
UI served from the board's own access point.

Every channel is an independent object: its own pin, its own mode, its own
frequency band, its own attack/decay memory and its own glow settings. By default
the spectrum is split across them — bass drives red, mids green, highs blue — so
the colour of the light follows the music.

Beat detection runs on the board from the spectral flux of each frame and reports
both an onset pulse and a BPM estimate, which any channel can use to flash on the
beat or to sync its glow to the tempo.

> Further ideas — VAD, a semantic "AIqualizer", OSC control, a manual colour pad
> and more — live in [IDEAS.md](IDEAS.md).

```
                   +--> 30 log bands (65 Hz - 7.6 kHz) --> LCD bars
PDM mic -> 512 @ 16 kHz
                   +--> broadband RMS level (instantaneous)
                   |
                   +--> band energy per light: center +/- width/2
                                   |
              +--------------------+--------------------+
              |                                         |
        Reactive mode                              Glow mode
   attack/release smoothing                  raised cosine, period
        gate -> gamma                             min..max
              |                                         |
     PWM on G32 / G33 / G26 -> R / G / B
```

The analyzer reports *unsmoothed* values: all time-based shaping happens in the
`Led` objects, which work from real elapsed time, so the decay and the glow
period stay accurate no matter how the loop is paced.

## Configuration lives in one file

[src/config.h](src/config.h) holds everything that depends on how the board is
wired, so moving a light is a one-line edit:

| Constant | Default | Meaning |
|----------|---------|---------|
| `kLedPins` | `{32, 33, 26}` | One PWM pin per channel (R / G / B) |
| `kLedChannels` | `{0, 1, 2}` | LEDC channel per channel |
| `kLedPwmFreq` | 5000 | PWM frequency in Hz |
| `kLedPwmBits` | 10 | PWM resolution, so duty is 0..1023 |
| `kSampleRate` | 16000 | Microphone sample rate |
| `kFftSamples` | 512 | FFT length, power of two |
| `kAudioMinHz` / `kAudioMaxHz` | 65 / 7600 | Analysis range, also the band limits |
| `kLedGamma` | 1.6 | Perceptual correction of the PWM duty |

The build asserts that there is exactly one LEDC channel per pin, so adding a
fourth channel means adding one entry to each array — the screen layout, the web
page and the JSON all size themselves from `kLedCount`.

> Channel choice matters: M5Unified already uses LEDC channel 6 for the red LED /
> IR emitter on GPIO19 and channel 7 for the LCD backlight. Channels 0-2 stay
> clear of those.
>
> One caveat: M5Unified takes G25 and G26 for the external speaker HAT if you
> enable `cfg.external_speaker.hat_spk2`. It is off by default — leave it off
> while blue is wired to G26.

## Wiring

Two channels are on the **Grove connector**, the third is on the **8-pin HAT
header**:

| Channel | Pin | Where |
|---------|-----|-------|
| R | **G32** | Grove, yellow wire |
| G | **G33** | Grove, white wire |
| B | **G26** | 8-pin HAT header |

M5Stack's pinmap for this board gives the HY2.0-4P port as `GND / 5V / G32 / G33`,
and lists `G0, G25, G26, G36` as the header pins — so **G26 is not reachable on
the Grove connector**. Check the header silkscreen for the exact pin order on your
unit.

All three are output-capable and reach the LEDC peripheral through the GPIO
matrix. Do not use the Grove **5 V** line to feed the LEDs.

### Prototype with plain LEDs

```
G32 ----[ 220R .. 330R ]----|>|---- GND        (R, Grove yellow)
G33 ----[ 220R .. 330R ]----|>|---- GND        (G, Grove white)
G26 ----[ 220R .. 330R ]----|>|---- GND        (B, HAT header)
GND  ---- Grove pin 1 (black wire) or any GND
```

Keep each GPIO below ~12 mA; that is fine for three indicator LEDs.

### Later, with MOSFETs (low-side switching)

```
G32 --[100R]-- Gate
                |
             [10k]  gate pulldown to GND
                |
GND ------------+---- Source
                   N-channel logic-level MOSFET (AO3400, IRLZ44N, ...)
                      Drain ---- strip cathode (-)
                      strip anode (+) ---- external supply +
                      external supply ------- GND (shared with the StickC)
```

Repeat for G33 and G26. Use logic-level devices (they must switch fully at
Vgs = 3.3 V). Each strip needs its own supply with a common ground, never the
board's 5 V pin. The 5 kHz PWM suits MOSFETs; drop to 1-2 kHz for very long or
capacitive loads.

## Build and flash

```sh
pio run              # build
pio run -t upload    # flash
pio device monitor   # 115200 baud
```

The board definition in [boards/m5stick-c-plus2.json](boards/m5stick-c-plus2.json)
adds the 8 MB flash of the Plus2, which PlatformIO's stock `m5stick-c` board does
not describe.

## Phone control over Wi-Fi

| | |
|---|---|
| Network | `SoundLight` |
| Password | `soundlight` |
| Page | <http://192.168.4.1> |

Connect the phone to that network and open the address; the screen shows the same
details for a few seconds at boot. Both values are compile-time constants in
[webui.h](src/webui.h) — change `kSsid` if you run two units side by side.

Pick the channel with the **R / G / B** buttons. The convention is:

* **Thick blue ring** — the channel you are currently editing.
* **Green fill** — that channel is powered on.
* **Red fill** — that channel is powered off.

The panel below repeats it in words, e.g. *Channel R — powered on · reactive ·
150 Hz*, so the ring is never the only clue. Each output meter is drawn in its
channel's colour.

> The access point is by far the biggest power consumer here; with the 200 mAh
> battery expect a bit over an hour of untethered use. Hold **button B** to turn
> it off (and on again) when you don't need the remote.

## Modes and parameters

Each light has its own complete set. Everything below is applied immediately.

### Power switch (per light)

Every light has its own **Power** switch at the top of its card, so you can kill
one output while leaving the other running — or park a light between songs without
losing its settings. A switched-off light is labelled `(off)` in the selector and
its output meter reads `off`.

Turning a light back on resumes rather than restarts: the smoothing and the glow
phase keep tracking while it is off, so it comes back in sync with the music
instead of jumping from a stale value. On the device, a switched-off light shows
`-` instead of `R`/`G` and its band marker turns grey.

### Reactive (sound → light)

| Parameter | Range | What it does |
|-----------|-------|--------------|
| Center frequency | 65 – 7600 Hz | Middle of the window this light listens to |
| Band width | 40 – 16000 Hz | Total width of that window, so it spans center ± width/2 |
| Attack | 0 – 1000 ms | How fast the light rises. `0` = follow the beat instantly |
| Release / decay | 0 – 2000 ms | Exponential decay when the sound drops. This is the "memory" |
| Noise gate | 0 – 50 % | Level below which the light stays completely dark |
| Echo repeats | 0 – 10 | How many times each trigger is replayed. `0` = off |
| Echo spacing | 20 – 500 ms | Time between repeats |
| Echo decay | 0 – 90 % | Each repeat keeps this share of the previous one |

The band limits what the light *hears*: with center 1000 Hz and width 500 Hz it
reacts to 750–1250 Hz only, and a loud kick at 60 Hz does nothing to it. The page
shows the effective window ("listens to 750 - 1250 Hz") as you drag.

**A window wide enough to cover the whole spectrum behaves as a broadband
reaction** — no separate switch. The band measurement is calibrated so a
full-width window reads the same as the broadband level, which means the same
gate and attack values work whether a light is narrow or wide.

Both times are *time constants*: with release at 200 ms, the light falls to 37 %
of its excursion 200 ms after the sound stops, and 13 % after 400 ms. Setting
either to 0 removes the memory entirely.

#### Echo

Each trigger is replayed a few times, like a delay with feedback: with 3 repeats,
120 ms spacing and 60 % decay, a flash at full brightness is followed by echoes at
60 %, 36 % and 22 %, spaced 120 ms apart. The repeats reuse the normal release
shape, so they look like the original flash rather than a separate effect.

The trigger is an **onset**: the sound must rise well above the level the
detector last settled at, and fall back before it can fire again. That means a
sustained tone fires once instead of machine-gunning, and the echoes themselves
can never retrigger the detector. The spacing also acts as the refractory period.

Echo is a reactive-mode feature; glow ignores it.

### Beat detection (per channel, shared detector)

| Setting | Range | What it does |
|---------|-------|--------------|
| Beat detection | Off / Flash / Sync glow | How this channel uses the beat |
| Beat flash strength | 0 – 100 % | In `Flash` mode, how much each beat adds |
| Beat sensitivity (shared) | 0 – 100 % | How eagerly a rise counts as a beat |

* **Flash** — every beat adds a quick flash on top of the reactive level, so the
  channel still follows its frequency band but punches on the beat.
* **Sync glow** — the glow period follows the detected tempo and the cycle
  restarts on each beat, so the brightest point lands on the beat instead of
  drifting.

The detector works on **spectral flux** (the summed positive change of the
spectrum between frames), picks beats with an adaptive `mean + k·σ` threshold, and
estimates tempo from the median of the last eight inter-beat intervals. Only one
detector runs (there is one microphone), so sensitivity is shared while each
channel decides what to do with the result. Measured on synthetic click tracks it
reports 117 BPM for 120, 89 for 90 and 144 for 140, and stays silent in silence.

### Glow (breathing)

| Parameter | Range | What it does |
|-----------|-------|--------------|
| Period | 100 – 5000 ms | One full dim → bright → dim cycle |
| Dim level | 0 – 100 % | Bottom of the cycle |
| Bright level | 0 – 100 % | Top of the cycle |

The sweep is a raised cosine, so it eases in and out instead of hitting a corner
at each end. Glow ignores the microphone and the frequency band entirely.
Switching modes restarts from the dim end, so there is never a jump.

### Microphone gain (shared)

`0` / `6` / `12` / `18` dB, applied before the analysis. There is one microphone,
so this is global rather than per light.

## Presets and persistence

Everything is stored in NVS, so the board comes back exactly as you left it,
including all three channels and both presets.

* **Two preset slots.** *Save 1 / Save 2* store the whole configuration (all
  channels, all parameters), *Load 1 / Load 2* bring it back. A slot that has never
  been saved has its *Load* button greyed out.
* **Factory reset** restores the documented defaults and saves them: R on the
  bass, G on the mids, B on the highs. The two presets are deliberately **left
  untouched**, so a reset cannot silently destroy stored setups.
* **Autosave** is debounced: a change is written once things have been quiet for
  800 ms, rather than on every slider movement, to spare the flash. Changes made
  with the buttons are saved the same way.

Storage details: namespace `sndlight`, keys `live`, `preset0`, `preset1`, each a
69-byte versioned blob (3 bytes of header plus 22 bytes per channel). An unknown
version or wrong length is rejected and the defaults are used instead, and values
read back from flash are clamped — a corrupted entry can never produce an
out-of-range configuration. Blobs from older firmware versions (one or two
channels, or before the power switch) are rejected, so upgrading gives you the
new defaults rather than a misread layout.

## Buttons

| Input | Action |
|-------|--------|
| Button A | Toggle R between reactive and glow |
| Hold button A | Same for G |
| Button B | Cycle microphone gain (`+0` / `+6` / `+12` / `+18` dB) |
| Hold button B | Toggle the access point |

There are only two usable buttons, so B (the third channel) is switched from the
phone or the API. The power switch of every channel is also phone-only.

## On-screen layout (240 x 135, landscape)

* Top line: each channel's mode (`R`/`G`, or `-` when switched off) and its
  window, e.g. `1:R150 2:R1.0k 3:R4.0k`
* Middle: 30 spectrum bars, green at the bottom to red at the top
* Three thin markers under the bars show **where each channel listens** in the
  spectrum, in that channel's colour (grey when it is switched off)
* Three output meters, one per channel, in that channel's colour
* Bottom line: broadband level, each channel's brightness and the tempo, e.g.
  `-38dB  61/34/ 0% 120bpm`
* Top right: `AP`, green while the access point is running

The serial port prints the same values once per second.

## HTTP API

Handy for scripting or for checking the board with `curl`:

```sh
curl http://192.168.4.1/api/state
# {"gainDb":0,"leds":[{"mode":0,"enabled":1,"centerHz":4000,...,"brightPct":61},...],
#  "levelPct":42,"dbfs":-38,"peakHz":440,"hasPreset0":1,"hasPreset1":0}

# light parameters need led=0 (R), 1 (G) or 2 (B)
curl -X POST 'http://192.168.4.1/api/params?led=1&centerHz=150&widthHz=250'
curl -X POST 'http://192.168.4.1/api/params?led=2&mode=1&glowPeriodMs=1500'
curl -X POST 'http://192.168.4.1/api/params?led=0&mode=0&attackMs=0&releaseMs=600&gatePct=8'
curl -X POST 'http://192.168.4.1/api/params?led=0&echoCount=4&echoSpacingMs=120&echoFeedbackPct=60'

# switch a light off / on
curl -X POST 'http://192.168.4.1/api/params?led=0&enabled=0'
curl -X POST 'http://192.168.4.1/api/params?led=0&enabled=1'

# gain is global
curl -X POST 'http://192.168.4.1/api/params?gainDb=12'

# beat detection: per-channel mode (0 off, 1 flash, 2 sync glow) and shared sensitivity
curl -X POST 'http://192.168.4.1/api/params?led=0&beatMode=1&beatBoostPct=80'
curl -X POST 'http://192.168.4.1/api/params?beatSensitivityPct=70'

# presets (1-based, matching the buttons)
curl -X POST 'http://192.168.4.1/api/preset?slot=1&action=save'
curl -X POST 'http://192.168.4.1/api/preset?slot=2&action=load'

# back to the defaults (presets are kept)
curl -X POST 'http://192.168.4.1/api/reset'
```

Unknown parameters are ignored and out-of-range values are clamped, so a bad
request can never reach the PWM stage. The response is the full state after the
change; a failed preset operation answers `400` with the unchanged state.

## Tuning beyond the web UI

Hardware constants are in [config.h](src/config.h) (see above). The remaining
behaviour constants are in [audio_fft.cpp](src/audio_fft.cpp):

| Constant | Meaning |
|----------|---------|
| `kBandFloorDb`, `kBandRangeDb` | dBFS window mapped onto the bar height |
| `kLevelFloorDb`, `kLevelFullDb` | dBFS window mapped onto the 0..1 control level |
| `kBandPowerNorm` | Normalisation that makes a full-width window match the broadband level |

For a finer low end, raise `kFftSamples` to 1024 (15.6 Hz bins, ~64 ms frames);
for a faster response, go the other way. It must stay a power of two.

## Notes

* The audio path is paced by the microphone: `update()` blocks for one frame
  (~32 ms), so the display refreshes at about 30 fps and the web page polls at
  400 ms. `handleClient()` runs once per frame, which is plenty for a phone.
* Only the difference between frames is painted for the bars, the markers and the
  meters, which keeps the screen free of flicker and the SPI traffic low.
* M5GFX interprets colour values in the panel's own depth, so the colours here
  are RGB565 (the ST7789 writes 16 bit).
* Microphone capture uses M5Unified's PDM support (`GPIO34` data, `GPIO0` clock)
  which is already configured for this board, so no pin setup is needed.

## Where this could go

Beat detection and BPM are done; the ideas that build on them — voice activity
detection, a semantic "AIqualizer", OSC control from a laptop, a manual colour
pad, and glow modulated by loudness or pitch — are written up with feasibility
notes in [IDEAS.md](IDEAS.md).
