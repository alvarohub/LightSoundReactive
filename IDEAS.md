# Ideas & Roadmap

Working notes for the sound-reactive light. Everything here is a candidate, not a
promise — the **Status** column says where each one stands.

The repository name is the ambition: react not only to *features of the signal* but
to **musical structure**, and eventually to **meaning**.

| # | Idea | Level | Status |
|---|------|-------|--------|
| 1 | Beat detection | rhythm | **Done** — onset + tempo, per-channel flash or glow sync |
| 2 | Harmonic analysis: chords, keys, scales | musical structure | Idea — *mostly feasible on the board* |
| 3 | Voice activity detection (VAD) | speech | Idea |
| 4 | "AIqualizer" — semantic / prosody equalizer | meaning | Idea |
| 5 | Remote control API (OSC over Wi-Fi) | plumbing | Idea |
| 6 | Manual mode — tap pad, 2D colour trajectories | interaction | Idea |
| 7 | Glow modulated by loudness / pitch / beat | musical structure | **Partly done** (beat sync) |
| — | [FFT backend choice](README.md#fft-backends) (ESP-DSP / arduinoFFT) | plumbing | **Done** |

---

## The arc: from signal to meaning

Worth keeping in view, because it explains the name and orders the work. Each rung
uses the one below, and each needs *less* of the signal and *more* context:

| Level | Question asked | Primitive | Where it runs |
|-------|----------------|-----------|---------------|
| 0. Signal | how loud, what spectrum? | RMS, FFT, band energy | **board** (done) |
| 1. Rhythm | when does something happen? | onset, tempo, downbeat | **board** (done) |
| 2. Harmony | what notes, what key, what mood? | chroma, chord templates, key profiles | **board**, mostly |
| 3. Speech | is someone talking, how are they talking? | VAD, pitch contour, energy, rate | host |
| 4. Meaning | what is being said, what does it intend? | ASR, sentiment, semantics | host / LLM |

The useful observation: **levels 0–2 fit on this board**, and level 2 is where
"musical meaning" starts — a major chord and a minor chord are already
*interpretation*, not measurement. Levels 3–4 need a computer, which is fine: the
board stays the real-time actuator and the host adds the slow layer.

A parallel that may be worth stealing: harmony is to music what syntax is to
language. Pitch classes are letters, chords are words, progressions are sentences,
and a key change is a change of topic.

---

## 1. Beat detection

### What shipped

The chain is the standard first stage of a music onset detector, running on the
board at ~31 fps:

```
audio frame -> FFT -> spectral flux -> adaptive threshold -> onset
                                            |
                            inter-onset intervals -> median -> BPM
```

* **Onset function**: spectral flux, the summed *positive* frame-to-frame change
  of the magnitude spectrum. Only rises count, so a decaying note is not
  mistaken for a new hit.
* **Threshold**: adaptive, `mean + k·σ` over a ~900 ms window. `k` comes from the
  Beat sensitivity slider (3.2σ at 0 %, 0.6σ at 100 %), so it adapts to loudness
  instead of needing a fixed level.
* **Refractory**: 180 ms minimum interval, which rejects the double triggers that
  a single drum hit produces. It doubles as the ceiling of 333 BPM.
* **Tempo**: median of the last 8 inter-onset intervals, folded into 60–170 BPM
  so a missed or doubled beat does not report 2× or 0.5× tempo.
* **Per channel**: `Off`, `Flash` (each beat adds a flash on top of the level) or
  `Sync glow` (glow period follows the tempo and peaks on the beat).

Measured on synthetic click tracks: 120 BPM → 117 reported, 90 → 89, 140 → 144,
no false beats in silence, and a sustained tone fires once rather than
continuously. The residual error is the 32 ms analysis grid.

### Where it could go next

* **SuperFlux / complex-domain onset detection** — the current flux is the 1990s
  baseline. SuperFlux (maximum filter over frequency + a vibrato-suppressing
  reference frame) reduces false onsets from vibrato and sustained notes, at the
  cost of one more full magnitude buffer (~2 KB) — affordable.
* **Better tempo tracking** — the median-of-intervals approach is simple. A comb
  filter over an onset-strength envelope, or dynamic-programming beat tracking
  (Ellis 2007), gives a tempo plus a *phase*, i.e. it predicts the next beat
  instead of only reacting. That would let the first beat after a pause land
  correctly.
* **Downbeat / bar detection** — needs a longer history and usually a
  spectral-template match over 4/4. Nice for "change colour every bar".
* **Per-band beat detection** — right now one detector listens to the whole
  spectrum. Running three (bass / mid / treble) would let the red channel follow
  the kick and the blue one the hi-hat *independently as beats*, not just as
  band energy.
* **Beat-synchronous parameter changes** — quantise a change (e.g. preset switch)
  to the next beat so transitions are musical.

---

## 2. Harmonic analysis: chords, keys, scales

**Idea**: agree on *what is being played*, not just how loud. Extract pitch classes,
name the chord, detect the key and mode, follow modulations. Then map harmony to
colour: major → warm and open, minor → cool and closed, a modulation → a colour
shift, a dissonance → tension.

**Why it is the interesting middle rung**: it is interpretation rather than
measurement, and — unlike speech — it is cheap enough to run on the board.

**How it works**

1. **Chroma** — fold the FFT bins into 12 pitch classes (A, A♯, B, …). One 12-float
   vector per frame: the harmonic content, octave-independent.
2. **Chord** — match the chroma against templates (major, minor, diminished,
   seventh) by cosine similarity. The best template above a threshold is the chord.
3. **Key and mode** — correlate a longer chroma average against
   Krumhansl-Schmuckler key profiles; the best of 24 gives key + major/minor.
4. **Dynamics** — smooth over a few frames, then emit *events* ("C→G→Am→F",
   "modulated to the relative minor") rather than a stream of labels.

**Feasibility on this board**

| Step | Cost | Verdict |
|------|------|---------|
| Chroma from the existing FFT | trivial — just re-bin | **yes** |
| Chord templates (12 × 4 dot products) | microseconds | **yes** |
| Key profiles over a 2–4 s window | microseconds | **yes** |
| Chord *segmentation* (beat-synchronous) | small | **yes** |
| Deep chord models (CNN/CRNN) | MBs, needs a runtime | host only |

So a useful subset runs on the ESP32 as it is: chroma, chord, key. Expect roughly
70–85 % chord accuracy on clean polyphonic music with plain templates — good for a
light show, not for transcription. A host-side model (madmom, Essentia) does much
better and would arrive over the network (idea 5).

**Mappings worth trying**

* **Key → palette**: C major warm, A minor cool; change palette on modulation.
* **Chord quality → movement**: major = smooth swell, minor = slow fade,
  diminished = flicker.
* **Chord change → colour jump**, vs **beat → brightness pulse**: two independent
  axes, so the light can be "in G minor at 128 BPM" rather than just "loud".
* **Scale/mode → tempo of the breathing**: Ionian calm, Dorian melancholy,
  Phrygian darker still — pop-music folklore, but it *reads* well.

**Pitfalls**

* Chroma needs reasonable frequency resolution at the low end: at 31 Hz/bin the
  bass is coarse; a 1024-point FFT (15.6 Hz) helps and still fits.
* Percussion smears chroma — compute it from a pitch-salience-weighted spectrum, or
  suppress transient frames.
* Tuning: if the source is not at A440 the chroma smears across bins. Estimate
  tuning (a global bin shift) before folding.
* Chord vocabulary is a taste decision: 24 triads is robust; adding sevenths and
  inversions multiplies confusion.

**Next step (small)**: chroma + 24-triad matching on the board, exposing
`/api/state` fields `chroma[12]`, `chord`, `key`. Then a `Chroma` display mode on
the LCD — twelve bars are a much more legible picture of the music than thirty
spectrum bands.

**Tools**: [librosa](https://librosa.org) (`chroma_stft`, `chroma_cqt`) and
[Essentia](https://essentia.upf.edu) for prototyping and ground truth;
[mir_eval](https://github.com/craffel/mir_eval) for scoring chord accuracy.

---

## 3. Voice activity detection (VAD)

**Idea**: only react to *voice*, ignoring music, clatter and traffic. The
reference is [SileroVAD](https://github.com/snakers4/silero-vad), a small,
well-regarded ONNX model.

**Why it is interesting**: a light that responds to speech but not to background
noise changes the installation completely — it becomes a "listen to me" object
rather than a music visualiser. It also unlocks idea 4 (prosody analysis needs to
know when someone is actually talking).

**Feasibility**

| Platform | Verdict |
|----------|---------|
| ESP32 (this board) | **No.** SileroVAD is ~1-2 MB and needs an ONNX/torch runtime plus float32 ops. 320 KB RAM, no FPU acceleration for this workload, and the 512-sample frames we already compute would have to be re-framed to exactly 30 ms. Not worth it. |
| Raspberry Pi / Jetson | **Yes**, trivially — 30 ms frames at ~1 % CPU. |

**So**: run the VAD on the host and send a single boolean over the network (see
idea 5). The ESP32 keeps doing what it is good at (PWM, display, low latency).

**Alternatives if it must be standalone**

* **Energy + ZCR + spectral flatness gate** — 50 lines, already 80 % of the way
  there with the existing gate. Cheap, no model.
* **WebRTC VAD (GMM)** — ~100 KB, fixed-point, has an ESP32 port in some forks.
  Worse than Silero but plausible.
* **A tiny custom CNN** — a 2-layer conv net on 13 MFCCs can reach ~90 % on a
  narrow task if trained on *your* room. TFLite Micro can run it in ESP32 PSRAM.
  Weeks of work, and only if the project deserves a dedicated model.

**Recommendation**: energy/VAD gate on-board for a quick win, Silero on the host
when the network link exists.

---

## 4. "AIqualizer"

**Idea**: an equalizer that does not just boost/cut frequencies but understands
the audio. Prosody (pitch contour, energy, tempo of speech), emotion, and even
the semantics of the words could drive both the light *and* the audio itself.

**Two very different products share the name**

1. **Visual AIqualizer** (safe, immediate): map emotion / prosody / topic to
   colour, brightness, movement. "Love" → warm slow breathing; "anger" → hard
   red flashes; questions → rising blue sweep.
2. **Audio AIqualizer** (the fun/risky one): actually process the sound — add
   reverb to words about love, duck the music under speech, mute slurs.

**Architecture** (the board cannot do this alone, and should not try)

```
mic ──► host (Python, 20-50 ms frames)
          ├── ASR (whisper.cpp, faster-whisper)        ─┐
          ├── prosody (F0 via YIN/CREPE, energy, rate)  ├─► feature vector
          ├── emotion (small classifier / LLM)          │
          └── VAD (idea 3)                             ─┘
                            │
                    rules or a learned mapping
                            │
              OSC/UDP ──► ESP32 (brightness, colour, mode)
                            │
                    optional: audio path
              DSP graph (pedalboard / JUCE) on the host
```

**Latency reality check**: ASR is 100-500 ms behind real time. That is fine for
*colour and mood* (the eye tolerates it) but useless for *beat-accurate* effects.
So split the work: the ESP32 keeps the low-latency reactions (level, beat), the
host adds the slow semantic layer on top. That hybrid is the sweet spot.

**Practical build order**

1. Host-side prosody only (F0 + energy + speaking rate) → OSC → light. No ASR,
   no LLM, ~1 day of work, and it already looks smart.
2. Add a lightweight emotion classifier (or just valence/arousal).
3. Only then consider semantics, and treat word-level reactions as a *party
   trick* with a deliberate 1-2 s delay, framed as "the light is thinking".

**Ethics / taste note**: muting or altering words based on their meaning is a
content-moderation policy, not a light effect. It will be wrong sometimes, and
"offensive" is culture-dependent. If it ever ships beyond this room, make the word
list explicit, visible and user-editable — and never silently alter someone
else's speech.

---

## 5. Remote control API (OSC over Wi-Fi)

**Idea**: the computer takes full control — switch modes, set every parameter in
real time, and select between `auto`, `idle` and `taken over` states.

**Why OSC**: it is the lingua franca of music software (TouchDesigner, Max/MSP,
SuperCollider, Ableton via Max4Live, Processing, Unity), it is trivial to send,
and it is UDP so there is no connection state to babysit. Every creative-coding
tool speaks it.

**Sketch**

| Address | Payload | Meaning |
|---------|---------|---------|
| `/led/0/level` | float 0..1 | direct brightness override |
| `/led/0/band` | float center, float width | set the frequency window |
| `/led/0/mode` | int | reactive / glow / off |
| `/beat` | float pulse, int bpm | host-detected beat (idea 1 on better hardware) |
| `/mode` | string | `auto` / `idle` / `remote` |
| `/preset/load` | int | load a stored preset |
| `/all/state` | — | request a state dump |

**Design points worth deciding early**

* **Ownership / arbitration**: when the host takes over, what happens to the
  local web UI and the buttons? Simplest sane rule: `remote` wins until a
  timeout (say 5 s without messages) or an explicit `/mode auto`. That way a
  crashed laptop cannot leave the lights stuck.
* **Transport**: OSC over UDP is easiest. Note the ESP32 will be in AP mode — the
  host must join `SoundLight`, so it loses internet. AP+STA mode fixes this at
  the cost of a second radio context and more power.
* **Smoothness**: send sparse control values, not per-frame brightness. The
  ESP32 already has attack/release smoothing; flooding it at 60 Hz fights that.
* **Discovery**: mDNS (`soundlight.local`) so the host does not hardcode
  192.168.4.1.

**Cheaper first step**: the HTTP API already exists and covers every parameter.
A `/api/params` POST per change is ugly but works today — worth a Python
`requests` wrapper before committing to an OSC transport.

**Libraries**: [OSC-for-Arduino](https://github.com/CNMAT/OSC) plus
`WiFiUDP`/`AsyncUDP`, ~20 KB of flash.

---

## 6. Manual mode

**Idea**: hands-on control for performance, not just automatic reaction.

* **Tap pad** — tap a button, the light flashes; taps set the tempo (the same
  BPM estimator as idea 1 can consume *tap* times instead of audio onsets, which
  is a nice reuse).
* **2D colour pad** — drag a finger over a plane (e.g. x = hue, y = brightness),
  the light follows in real time.
* **Trajectory recording** — the pad records the path and loops it, like a
  Kaoss Pad. Add a decay so the loop fades if you stop drawing.

**On the ESP32**: the web page can host the pad (touch/drag events over
WebSocket or polling). The board only stores a short path — a few hundred points
is 2-3 KB, fine. A "shape" is then: a list of (t, x, y) points with a duration,
played back with interpolation.

**Cheaper first step**: because the HTTP API already accepts per-channel colour
parameters, a desktop prototype (a 2D pad in Processing or a browser page) can
drive the light *today*, which validates whether the interaction is fun before
any firmware work. The natural place for this is TouchDesigner/OSC (idea 5).

---

## 7. Glow modulated by the music

**Done**: `Sync glow` ties the glow period to the detected tempo and restarts the
cycle on each beat, so the breathing peaks land on the beat.

**Still open**

* **Nervousness → period**: derive a "busy-ness" metric (spectral flux variance
  or note density) and shorten the period for frantic music, lengthen it for
  ambient. The user's instinct — "high-pitched music is more nervous" — maps
  well to spectral centroid, which we already have the ingredients for.
* **Loudness → amplitude**: map the level to glow min/max so quiet passages
  breathe subtly and loud ones swing wide. Easy, no new DSP.
* **Pitch → hue**: for each channel, shift the colour with the spectral
  centroid. Currently "colour" means "which physical LED", so this only makes
  sense once we have an RGB *strip* (one pin, addressable) rather than three
  separate LEDs — see below.

---

## Cross-cutting: hardware & architecture

The three-LED R/G/B setup is three independent single-colour channels. A
**WS2812 / SK6812 addressable strip** would change the game: one pin, arbitrary
colour per pixel, and suddenly ideas 4 and 7 (hue, gradients, per-pixel
visualisation) become easy. The ESP32 drives these comfortably with RMT.

**Split of responsibilities** as the project grows:

| Layer | Runs on | Latency budget |
|-------|---------|----------------|
| Level, band, beat | ESP32 | < 40 ms (already there) |
| VAD, prosody, emotion | RPi / laptop | 50-300 ms |
| ASR, semantics, audio DSP | laptop / Jetson | 0.5-2 s |

The ESP32 should stay the real-time actuator. Anything that needs a model goes
over the wire, and the link degrades gracefully: if the host disappears, the
board keeps reacting to sound on its own.

---

## Suggested next three

1. **Harmony on the board** (idea 2): chroma + chord + key. It is the cheapest route
   to actual *musical meaning*, needs no network, and reuses the FFT that already
   runs — the only reason it is not first is that it wants a chroma display to be
   worth looking at.
2. **Host-side prosody → OSC** (ideas 4+5 combined): highest ratio of "wow" to
   effort, and it validates the remote-control architecture the other ideas depend
   on.
3. **Per-band beat detection**: three detectors instead of one, so R/G/B can each
   react to their own beat — a small change to code that already exists.

Following close behind: an **addressable RGB strip**, which unlocks colour, and
that most of the visual ideas quietly assume.

---

## Notes to self

* Test with real music, not just synthetic clicks — click tracks flatter any
  beat detector. A drum-and-bass track and a piano piece are the two hard cases.
* Harmony needs a *reference*: validate chroma/chord code against librosa or
  Essentia on the same audio before trusting the on-board version.
* The AP costs battery. Anything network-based should have an obvious off switch
  (button B already toggles it).
* Keep the ESP32 firmware honest: every new feature should still work, at least
  in a basic form, with the network down.
