# Using it

[← back to the README](../README.md)

## The phone page

Open <http://192.168.4.1> and pick a channel with the **R / G / B** buttons:

| Signal | Meaning |
|--------|---------|
| **Thick blue ring** | The channel you are editing |
| **Green fill** | That channel is powered on |
| **Red fill** | That channel is powered off |

The panel below repeats it in words — *Channel R — powered on · reactive · 150 Hz* —
so the ring is never the only clue. Beneath that are three output meters in their
channel's colours, plus the live level, dBFS, peak frequency and BPM.

Everything applies immediately, and is saved automatically after 800 ms of quiet
(debounced, to spare the flash).

Every knob is listed in [Parameters](parameters.md).

## Buttons

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

## The screen (240 × 135, landscape)

```
1:R150 2:R1.0k 3:R4.0k               AP     ← mode + band per channel, Wi-Fi
|  | | | |  | | | |  |  |  |  |  |  |  |   30 spectrum bars
===========                                 ← where each channel listens
--   --      ----------                     (one thin marker per channel)
[=======        ]  R out
[=====          ]  G out                    ← output meters, one per channel
[               ]  B out
-38dB  61/34/ 0% 120bpm                     ← level, brightness, tempo
```

A channel shows `R` (reactive), `G` (glow) or `-` (switched off). The serial port
prints the same values once per second.

## Presets

* **Save 1 / Save 2** store the whole configuration — all three channels, every
  parameter.
* **Load 1 / Load 2** bring it back. A slot that was never saved is greyed out.
* **Factory reset** restores the defaults: R on the bass, G on the mids, B on the
  highs. It deliberately **keeps your presets**.

Settings live in NVS and survive a power cycle. A change takes up to a second to be
written, so give it a moment before pulling power.
