# Yokoko Onono Pitch Delay

A model of an analogue pitch-shifting delay: four MN3005 bucket-brigade chips (16,384 stages) on a microcontroller clock, with a compander, a resonant filter in the feedback loop and a three-step pitch sequencer.

## Getting the plugin (GitHub builds it)

1. Upload this folder's contents to a GitHub repository, including the hidden `.github` folder.
2. Open the **Actions** tab. "Build Windows VST3" runs by itself and takes about 10 minutes.
3. Open the finished run (green check) and download **YokokoOnono-VST3** under Artifacts.
4. Extract the zip and copy the **Yokoko Onono Pitch Delay.vst3** folder into `C:\Program Files\Common Files\VST3\`.
5. Rescan plugins in your DAW.

## The front

| Control | What it does |
|---|---|
| mix | Dry and wet. Noon is both at full. |
| filter | The resonant low-pass inside the feedback loop. Each repeat passes through it again. |
| feedback | How much comes back. Past about three quarters it starts to run away on its own. |
| glide | How long the clock takes to slide to a new interval. All the way down is an instant jump. |
| interval 1, interval 2 | The pitch of steps 2 and 3: 2 octaves down to 2 octaves up, same note at noon. |
| step 1/2/3 | How long each step lasts: quarter, dotted eighth or eighth. |
| memory | one, now, two. Pick one or two and press **save here** to keep the current settings there. Switching to it later brings them back. |
| time | Drag the number, or tap the **tap** circle. With "follow the song" on, it uses your DAW's tempo. |
| on | Effect on or off. |

How the pitch works: like the real hardware, one clock drives the chips. When a step changes the clock, whatever is already inside comes out faster or slower, so you hear it at the new pitch. With feedback up, each repeat gets shifted again on the next step.

## The back (click "turn it over")

- **follow the song**: use the DAW tempo. Tapping turns it off.
- **modulation**: interval 1 and 2 become LFO speed and depth. The step switches become shape (triangle, sine, square), randomness (none, some, lots) and what moves (pitch, filter, both).
- **step**: the sequence stops. Tap, or send a MIDI note, to move to the next step.
- **slowdown**: halves the clock. Up to 32 seconds of delay, with aliasing and clock whine.
- **ramp the filter / feedback / glide**: that knob sweeps up and down by itself. Mix becomes ramp speed.
- **trails**: repeats keep ringing when you switch the effect off.
- **momentary**: the on circle only works while held.

Every control can be automated in your DAW, which covers what the pedal's CV and expression inputs would do.

## Building it yourself instead

Install Visual Studio 2022 Community (Desktop development with C++), CMake 3.22+ and Git, then in this folder:

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Built with [JUCE](https://juce.com/) 9.0.5. Font: Courier Prime (SIL Open Font License, see `Resources/OFL.txt`).
