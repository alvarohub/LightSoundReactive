# Build and flash

[← back to the README](../README.md)

## Install PlatformIO

Either the [VS Code extension](https://platformio.org/install/extension) or the CLI:

```sh
pip install platformio
```

The Arduino framework and all libraries are fetched automatically on the first
build. No separate Arduino IDE setup is needed.

## Build and upload

```sh
git clone https://github.com/alvarohub/LightSoundReactive.git
cd LightSoundReactive

pio run                  # build
pio run -t upload        # flash over USB-C
pio device monitor       # 115200 baud: watch it talk
```

The first build downloads the ESP32 toolchain (~200 MB), so it takes a few minutes.
After that, builds take seconds.

## After flashing

The board starts a Wi-Fi access point and shows its details on the LCD for a few
seconds:

| | |
|---|---|
| Network | `SoundLight` |
| Password | `soundlight` |
| Page | <http://192.168.4.1> |

Connect a phone to that network and open the address. There is no internet on it —
the board is talking to you directly. Some phones silently fall back to mobile
data; if the page does not load, disable mobile data for a moment.

Serial output confirms the boot:

```
ready: 3 lights, fft 512/16000 Hz, ap http://192.168.4.1
  light 1: G32, ledc channel 0
  light 2: G33, ledc channel 1
  light 3: G26, ledc channel 2
```

## FFT backends

The FFT sits behind a small interface with two interchangeable implementations.
Pick one with the PlatformIO environment:

| Environment | Backend | Licence | RAM | Flash |
|---|---|---|---|---|
| `m5stick-c-plus2` (default) | ESP-DSP | Apache-2.0 | 18.7 % | 31.5 % |
| `m5stick-c-plus2-arduinodft` | arduinoFFT | GPL-3.0 | 17.5 % | 31.3 % |

```sh
pio run -e m5stick-c-plus2-arduinodft -t upload
```

**ESP-DSP** ships inside the Arduino-ESP32 core, so it needs no extra download, and
it dispatches to a hand-written assembly routine on ESP32. It costs ~4 KB more RAM
for a scratch buffer, and is the faster of the two.

**arduinoFFT** is the alternative: ~1 KB less RAM, but an extra dependency, and
because it is GPL-3.0 the resulting binary is GPL-3.0 too. The default
environment contains only MIT and Apache-2.0 code.

The two are numerically interchangeable — see
[Internals](internals.md#fft-backends) for why that needed care, and for the
measurements.

## Board definition

[boards/m5stick-c-plus2.json](../boards/m5stick-c-plus2.json) adds the 8 MB flash of
the Plus2, which PlatformIO's stock `m5stick-c` board does not describe.

## If the upload fails

See [Troubleshooting](troubleshooting.md) — the short version is: close
`pio device monitor` first (only one program can hold the port), and if the board is
not detected over a USB-C→C cable, power-cycle it.
