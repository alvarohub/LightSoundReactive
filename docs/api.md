# HTTP API

[← back to the README](../README.md)

The web page is just a client of these endpoints, so anything the page can do, a
script can do too. Plain HTTP on port 80, no authentication (it is a local access
point).

## Endpoints

| Method | Path | Purpose |
|--------|------|---------|
| `GET` | `/` | The control page |
| `GET` | `/api/state` | Full state as JSON (parameters + live values) |
| `POST` | `/api/params` | Change parameters, given in the query string |
| `POST` | `/api/preset?slot=1\|2&action=save\|load` | Presets |
| `POST` | `/api/reset` | Restore defaults (presets kept) |

`/api/params` accepts any number of parameters in one request. Channel parameters
need a `led=` index; global parameters do not.

## Per-channel parameters (`led=0` R, `1` G, `2` B)

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

## Global parameters

| Key | Range | Default |
|-----|-------|---------|
| `gainDb` | `0` / `6` / `12` / `18` (snapped) | 0 |
| `beatSensitivityPct` | 0–100 | 50 |

## State response

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
      "brightPct": 61, "duty": 637
    }
  ],
  "levelPct": 42, "dbfs": -38, "peakHz": 440,
  "bpm": 120, "beatPct": 35,
  "hasPreset0": 1, "hasPreset1": 0
}
```

`leds` has one entry per channel, in R / G / B order. `brightPct` and `duty` are
what that channel is outputting right now; `bpm` stays 0 until the detector has
heard a few beats.

## Examples

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

## Behaviour

* Unknown parameters are **ignored**; out-of-range values are **clamped**. A bad
  request can never reach the PWM stage.
* Every call returns the full state, so one round trip confirms the change.
* A preset operation that cannot be honoured answers `400` with the unchanged state
  (for example `action=load` on an empty slot).
* There is no push channel: poll `/api/state` for live values. The page polls every
  400 ms, which is plenty for a phone.

## Notes

* The board is in AP mode, so the machine calling it must be on the `SoundLight`
  network — and therefore has no internet at that moment.
* Ideas for a richer control protocol (OSC, host-side takeover) are in
  [IDEAS.md](../IDEAS.md).
