# LightSoundReactive

Real-time sound analysis driving an RGB light show on the **M5StickC Plus2** — a
matchbox-sized ESP32 board with a built-in microphone and screen.

The board listens to the room, splits the spectrum into three independent bands
(bass → red, mids → green, highs → blue), detects beats and tempo, and drives
PWM on three pins. You tune it from a phone, over a web page served by the board
itself. No app, no router, no cloud.

```
mic → FFT → per-band level ─┐
        └→ beat / tempo ────┼→ R, G, B  ← three PWM pins
                            └→ LCD spectrum
```

## Where this is going

This is the first rung of a bigger goal: not only reacting to **features of the
signal** (loudness, spectrum, onsets), but responding to **higher-level content**.

For music that means harmony — chords, keys, scales, modulation — which is where a
minor chord and a major chord start to *mean* different things. For voice it means
prosody, emotion and, eventually, semantics. The speech side is developed
separately in [voiceAnonymizer_PI](https://github.com/alvarohub/voiceAnonymizer_PI)
(real-time emotion and prosody analysis, OSC-controlled), because it needs more
compute than an ESP32 has.

What is here today covers the bottom of that ladder and runs entirely on the board.
The climb is mapped in [IDEAS.md](IDEAS.md).

## Quick start

```sh
git clone https://github.com/alvarohub/LightSoundReactive.git
cd LightSoundReactive
pio run -t upload        # needs PlatformIO; first build downloads the toolchain
```

The board then starts its own Wi-Fi network — **`SoundLight` / `soundlight`** — and
serves the control page at **<http://192.168.4.1>**. The same details appear on the
LCD for a few seconds after boot.

Full steps: [Build and flash](docs/build-and-flash.md).

## Documentation

| | |
|---|---|
| **[Hardware and wiring](docs/hardware.md)** | What to buy, which pins, LEDs, MOSFETs |
| **[Build and flash](docs/build-and-flash.md)** | PlatformIO setup, the two FFT backends, uploading |
| **[Using it](docs/usage.md)** | Phone page, buttons, screen, presets |
| **[Parameters](docs/parameters.md)** | Every knob, its range and its default |
| **[HTTP API](docs/api.md)** | Endpoints, parameter keys, JSON state |
| **[Internals](docs/internals.md)** | Pipeline, timing, storage, project layout, accuracy |
| **[Troubleshooting](docs/troubleshooting.md)** | When it does not do what you expect |
| **[Ideas and roadmap](IDEAS.md)** | From signal to meaning: what could come next |

## Credits

**Author:** Alvaro Cassinelli.

Built on [M5Unified / M5GFX](https://github.com/m5stack/M5Unified) (board support,
display, microphone capture), [ESP-DSP](https://github.com/espressif/esp-dsp) (the
default FFT) and optionally [arduinoFFT](https://github.com/kosme/arduinoFFT).
Thanks to all three.

## Licence

MIT — see [LICENSE](LICENSE). Use it, change it, sell it; keep the notice.

One caveat worth knowing: `arduinoFFT` is GPL-3.0, so a binary built with the
`m5stick-c-plus2-arduinodft` environment is GPL-3.0. The **default** environment
uses ESP-DSP and contains only MIT and Apache-2.0 code. Details in
[Build and flash](docs/build-and-flash.md#fft-backends).
