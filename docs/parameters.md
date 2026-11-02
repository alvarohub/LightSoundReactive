# Parameters

[← back to the README](../README.md)

Each channel has its own complete set. All values are live: they apply as you change
them, and the current set is visible in [the API](api.md).

## Reactive (sound → light)

| Parameter        | Range       | Default                 | What it does                                          |
| ---------------- | ----------- | ----------------------- | ----------------------------------------------------- |
| Center frequency | 65–7600 Hz  | R 150 · G 1000 · B 4000 | Middle of the band this channel listens to            |
| Band width       | 40–16000 Hz | R 250 · G 1400 · B 5000 | Total width, so it spans center ± width/2             |
| Attack           | 0–1000 ms   | 30                      | How fast the light rises. `0` = instant               |
| Release / decay  | 0–2000 ms   | 200                     | Exponential decay when the sound drops — the "memory" |
| Noise gate       | 0–50 %      | 4                       | Level below which the light stays dark                |
| Echo repeats     | 0–10        | 0                       | How many times each trigger is replayed               |
| Echo spacing     | 20–500 ms   | 120                     | Time between repeats                                  |
| Echo decay       | 0–90 %      | 60                      | Each repeat keeps this share of the previous          |

The band limits what the channel _hears_: center 1000 Hz with width 500 Hz reacts to
750–1250 Hz only, so a kick at 60 Hz does nothing to it. The page shows the effective
window ("listens to 750 - 1250 Hz") while you drag.

A band wide enough to cover the whole spectrum behaves as a **broadband** reaction —
there is no separate switch. The band measurement is calibrated so a full-width
window reads the same as the broadband level, which is why the same gate and attack
work whether a channel is narrow or wide.

Both times are **time constants**: with release at 200 ms the light falls to 37 % of
its excursion after 200 ms, and 13 % after 400 ms. Setting either to 0 removes the
memory entirely.

### Echo

Each trigger is replayed like a delay with feedback: 3 repeats, 120 ms spacing and
60 % decay gives a flash at 100 %, then echoes at 60 %, 36 % and 22 %. The repeats
reuse the normal release shape, so they look like the original flash rather than a
separate effect.

The trigger is an _onset_: the sound must rise well above where the detector
settled, then fall back. So a sustained tone fires once instead of machine-gunning,
and the echoes cannot retrigger it.

## Glow (breathing)

| Parameter    | Range       | Default | What it does                      |
| ------------ | ----------- | ------- | --------------------------------- |
| Period       | 100–5000 ms | 2000    | One full dim → bright → dim cycle |
| Dim level    | 0–100 %     | 0       | Bottom of the cycle               |
| Bright level | 0–100 %     | 100     | Top of the cycle                  |

The sweep is a raised cosine, so it eases in and out instead of hitting a corner at
each end. Glow ignores the microphone and the frequency band.

## Manual (fingertip drum pads)

The web page provides three circular **drum disks** (`R`, `G`, `B`) near the top of
the screen. Tapping a disk with your fingertip fires an immediate pulse (`POST /api/tap?led=0|1|2`)
with zero click delay and multi-touch support:

- **In `Manual` mode** (`mode = 2`), the channel ignores the microphone and stays
  dark until tapped (or flashed by the beat detector if `Beat effect: Flash` is on).
  Each tap jumps to 100 % and decays according to the channel's **Release / decay**
  (`releaseMs`) and **Echo** (`echoCount`, `echoSpacingMs`, `echoFeedbackPct`)
  settings.
- **In `Reactive` or `Glow` mode**, tapping a channel's disk still punches an
  immediate pulse + echo on top of whatever the channel is doing.

## Beat detection

| Setting             | Range                   | Default | Scope       |
| ------------------- | ----------------------- | ------- | ----------- |
| Beat detection      | Off / Flash / Sync glow | Off     | Per channel |
| Beat flash strength | 0–100 %                 | 60      | Per channel |
| Beat sensitivity    | 0–100 %                 | 50      | Shared      |

- **Flash** — works in **both** Reactive and Glow modes: each detected beat adds a
  quick flash (`Beat flash strength`) on top of whatever the channel is doing (its
  band-reactive level or its slow breathing wave).
- **Sync glow** — **Glow mode only** (disabled in Reactive mode): the breathing
  period locks to the detected tempo (`60000 / BPM` ms) and peaks on the beat
  instead of drifting. When no steady beat is present, it falls back to the manual
  `Period`.

Only one detector runs, because there is one microphone: sensitivity is shared, but
each channel decides what to do with the result.

## Microphone gain

`0` / `6` / `12` / `18` dB, applied before the analysis and shared by all channels.
Raise it if the bars sit flat in a quiet room.

## Power switch

Every channel can be switched off independently without losing its settings. A
switched-off channel keeps tracking internally (glow phase, smoothing), so switching
it back on resumes in sync rather than jumping.
