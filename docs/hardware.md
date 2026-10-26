# Hardware and wiring

[← back to the README](../README.md)

## What you need

| Part | Notes |
|------|-------|
| **M5StickC Plus2** (SKU K016-P2) | ESP32-PICO-V3-02, 8 MB flash, 2 MB PSRAM, 200 mAh battery |
| 3 × LEDs | Or one RGB module; or a strip later, with MOSFETs |
| 3 × resistors, 220–330 Ω | One per LED |
| 1 × Grove cable | Carries the first two channels |
| Wire for the third channel | It goes to the 8-pin header, not the Grove port |
| *Optional:* 3 × logic-level MOSFETs | For strips, or anything above ~12 mA |

The board's built-in **SPM1423 microphone** is what makes this possible with no
extra parts — no external mic, no I2S wiring.

## Pins

Two channels are on the **Grove connector**, the third on the **8-pin HAT header**:

| Channel | Pin | Where |
|---------|-----|-------|
| **R** | **G32** | Grove port, yellow wire |
| **G** | **G33** | Grove port, white wire |
| **B** | **G26** | 8-pin HAT header |

M5Stack's pinmap for this board gives the HY2.0-4P port as `GND / 5V / G32 / G33`
and lists `G0, G25, G26, G36` as the header pins, so **G26 is not reachable on the
Grove connector**. Check the header silkscreen for the exact pin order on your
unit.

All three are output-capable and reach the LED controller (LEDC) through the GPIO
matrix. **Never** power LEDs from the Grove 5 V pin.

To move a channel, edit `kLedPins` in [src/config.h](../src/config.h) — one line.

## Prototype with plain LEDs

```
G32 ----[ 220R .. 330R ]----|>|---- GND        (R, Grove yellow)
G33 ----[ 220R .. 330R ]----|>|---- GND        (G, Grove white)
G26 ----[ 220R .. 330R ]----|>|---- GND        (B, HAT header)
GND  ---- Grove pin 1 (black) or any GND
```

Keep each GPIO below ~12 mA. That is fine for three indicator LEDs, not for a
strip.

## Driving real lights with MOSFETs

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

Repeat for G32 and G33.

* Use a **logic-level** device: it must switch fully at Vgs = 3.3 V.
* Each strip needs its own supply, with a **common ground** to the board.
* 5 kHz PWM suits MOSFETs. For very long or capacitive loads, lower `kLedPwmFreq`
  to 1–2 kHz in [src/config.h](../src/config.h).

## Two things that can bite

* **LEDC channels 6 and 7 are taken.** M5Unified uses them for the red status LED /
  IR emitter on G19 and for the LCD backlight. The firmware uses 0–2.
* **G25 and G26 belong to the external-speaker HAT** if you enable
  `cfg.external_speaker.hat_spk2`. It is off by default — leave it off while blue is
  wired to G26.
