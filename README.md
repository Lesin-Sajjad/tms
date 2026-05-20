# TMS EMG Monitor

Real-time 4-channel EMG acquisition and visualization system built for TMS
(Transcranial Magnetic Stimulation) research in collaboration with NIMHANS. Uses
a CED Micro 1401 data acquisition unit to capture muscle activity (EMG) before
and after TMS pulses.

**Platform: Windows only** (CED drivers are Win32).

## Hardware

- **CED Micro 1401** — professional DAQ unit, 10kHz per channel, ±5V, 16-bit
- Electrode patches on target muscle groups; reference electrode on bony
  landmark

## Quick Start (Python)

All scripts run from the `python/` directory — the CED DLLs (`py1401.pyd`,
`1464ui.dll`) must be co-located.

```bash
cd python

# Live rolling plot (all 4 channels, ~500 Hz polling)
python fast.py

# All 4 channels superimposed on one axis
python superimposed.py

# 500ms block capture at true 10kHz, static plot
python ced_capture.py

# Text-mode live voltage readout (quick sanity check)
python verify_signals.py
```

Dependencies: `numpy`, `matplotlib`. `py1401` is the bundled extension module.

## C++ Monitor (Windows)

`c++/final.cpp` — Win32 GUI with real-time scrolling plot, X/Y zoom sliders,
Space to pause.

```
cl final.cpp Use1432.lib comctl32.lib winmm.lib /Fe:final.exe
```

Place `Use1432.dll` in the same directory as the built `.exe`.

`c++/plot_zoomed.cpp` is an enhanced version with double-buffered rendering and
resizable layout.

## Capture Modes

**Polling** (`fast.py`, `superimposed.py`): sends `adc,-3;` to the 1401 each
loop, reads 4 space-separated int16 values. Simple but USB-latency-limited
(~500–1000 Hz effective rate).

**ADCMEM block transfer** (`ced_capture.py`, C++ `final.cpp`): programs the 1401
to fill its internal memory at true 10kHz, then bulk-transfers via `ToHost`. Use
this for any timing-sensitive analysis.

## Key Notes

- Electrical interference (laptop chargers, nearby cables) is the biggest noise
  source — keep power supplies away from the electrode circuit.
- Reference electrode must be on a bony/non-muscular region; misplacement causes
  baseline noise that overlaps with signal peaks.
- Electrode patches degrade with reuse — replace for clinical recordings.
- Both 9V batteries powering the EMG module must stay above ~9V; low voltage
  causes complete signal loss.
