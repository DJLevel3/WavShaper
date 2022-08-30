# WavShaper
A 2D waveshaper for the oscilloscope music community!
--------

## Uh... what even is a 2D waveshaper?

Well, it's actually pretty simple, so let me explain.

A normal, mono audio signal is just a number changing over a time, from -1 to +1. There's some voltage involved, and other annoying stuff like sample rate, but internally in your computer, that's all it is. However, there are things you can do with a signal that goes from -1 to +1 other than putting them straight to the speakers! For instance, you could have the signal go around a 2D circle, where -1 and +1 are the same point on the circle, and 0 is halfway around. Then, the audio signal is instead signalling what point to output to the speakers. But the best part is...

### It still sounds like the original signal!!!

This is huge. It's not exactly like the original signal, there's some distortion, but if you put a voice in, you get a voice out, but on a circle!

Now, imagine that instead of a circle, you have something more complicated, such as the character 'h'. What happens when you put the audio signal as the point on the 'h' to send to the speakers? That's right, you get a signal that sounds a lot like the original, but looks like the 'h'! This can be extended to just about anything you can think of, with the caveat that a more complicated shape makes the output noisier and less intelligible.

Interestingly, if you fade bewteen the unshaped and the shaped audio, you get the unshaped audio to morph into the shaped audio! Neat, huh?

## Well, it's probably super complicated and expensive to compute, right?

Nope! The algorithm that runs the whole show is one of the simplest out there, the simple linear interpolation or 'lerp'. A lerp takes a start value, and end value, and a percentage value, and returns a value that is that percentage from the start to the end. And the algorithm can be simplified to a function:

```
float lerp(float start, float end, float t) {
  return (start + (end - start) * t);
}
```

The three operations done there are three of the fastest operations you can do on a computer. Division and square roots are extremely slow by comparison, but adding, subtracting, and multiplying are CRAZY fast.

## So, how do I get it?
Well, if you want it immediately, you can get the latest VST3 release [here](https://github.com/DJLevel3/WavShaper/releases/latest).

If you want to compile it, you'll need Visual Studio 2022 on Windows. Then, follow these steps:
1. Download [iPlug2](https://github.com/iPlug2/iPlug2)
2. Follow the [Windows setup instructions](https://github.com/iPlug2/iPlug2/wiki/02_Getting_started_windows), ending after setting up the development environment.
3. Clone or download the WavShaper repo into the Examples folder
4. Open WavShaper.sln
5. Set WavShaper-vst3 as the startup project
6. Build it! If you have permissions set so that all users have full control over the folder `"C:\Program Files\Common Files\VST3\"`, then it should™ install there.

## But, how do I use it?
It's pretty simple, actually.

I have included 2 examples, the circle and the 'h' character mentioned above, which are located in the `example-audio` directory. When you open the VST3 plugin in your DAW, just press the `Choose Shape...` button in the middle and pick one of the two. Enable the plugin with the `Enable` switch, and you're shaping!

The Input Gain knob changes the volume of the input signal into the shape, and the Output Gain knob changes the volume of everything just before sending the audio to the DAW.

The Shaping knob fades between the unshaped and the shaped signals as explained above, and the Offset knob shifts the input audio around the shape.

The Enable switch turns on shaping. If it's off, the audio goes right through and is only affected by the input gain. If it's on, everything affects the audio.

## What libraries and stuff does it use?

- WavShaper uses [iPlug2](https://github.com/iPlug2/iPlug2) for the UI and VST3 plugin framework.
- WavShaper uses [AudioFile](https://github.com/adamstark/AudioFile) to open and read .wav files.

## Who developed WavShaper?
- Me, DJ_Level_3!
- You, the reader, if you find and fix an issue! Just make an issue and/or a pull request!

## Known Bugs
- For some reason, [PrettyScope](https://www.soundemote.com/plugins/prettyscope) crashes the DAW entirely (at least in Ableton Live 11 Standard) if you add it immediately after a newly added WavShaper. I'm working on it with Soundemote, it may or may not get fixed. To avoid this, open PrettyScope first, then drag WavShaper in before it.
- There is a clicking sound if you enable the plugin, this is caused by the audio having DC offset when silent. I'm working on this, the fix just isn't done yet.
