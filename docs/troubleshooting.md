# Troubleshooting

[← back to the README](../README.md)

| Symptom | Likely cause / fix |
|---------|--------------------|
| Port not recognised over a USB-C→C cable | Known on this board. Unplug, power off (long-press power until the green LED lights), then reconnect. |
| Upload fails, "port busy" | Close `pio device monitor` first — only one program can hold the port. |
| Upload fails, board not responding | Hold the power button while starting the upload. |
| No `SoundLight` network appears | Hold **B** to enable the access point; check the serial output for `[web] ... -> http://192.168.4.1`. |
| Page will not load | Some phones fall back to mobile data on a network without internet. Disable mobile data, or set the Wi-Fi connection to "stay connected". |
| LEDs never light | Check the pin (B is on the header, not Grove), LED polarity, that the channel is **on**, and that the noise gate is not too high. |
| One channel looks dead in reactive mode | Its band may be narrow and the music may have nothing there. Widen `widthHz` to test. |
| Reacts to nothing, or almost nothing | Raise the microphone gain (button **B**) or lower the noise gate. |
| Beat flashes constantly on a busy track | Lower **Beat sensitivity**, or raise the noise gate. |
| BPM reads `--` | It needs a few clear beats; sparse or rubato material will not produce a tempo. |
| LED flickers or buzzes | Long wires or a capacitive load: lower `kLedPwmFreq` to 1–2 kHz. |
| Settings forgotten after reboot | The NVS write needs 800 ms of quiet after the last change — wait a second before pulling power. |
| Screen stays on the Wi-Fi banner | Normal for ~3 s at boot; it clears itself. |
| Battery dies in an hour | Expected with the access point on. Hold **B** to switch Wi-Fi off. |

## Still stuck?

`pio device monitor` shows what the firmware thinks it is doing: the boot summary
(pins, FFT size, AP address) and one line per second with level, brightness and
tempo. If the microphone failed to start it says `mic init failed`; if settings
could not be stored it says `settings will not persist`.
