Bmxplay
=======

Tiny player (up to 3 kb) for Jescola's Buzz by Joric^Proxium.
First used in 64k-intro [Yo-08][1] (2002).
Completely written in plain C (mostly in october 2001), considering a _future possibility_
of porting everything to assembly. Uses its own C++ classes implementation in plain C.
Used to patch virtual methods table in order to comply with various versions of Buzz.
If you want conventional or "modern" OOP, check out [flashbmxplay][2] or [bmxplayjs][3].

License
-------

Player is public domain. MachineInterface.h (written by Oskari) and (therefore) all dlls are for
non-commercial use. Note that player neither links them nor uses their API or header (they are
only for composing music in Buzz), so you can use the player in a commercial app
(or a keygen), without legal restrictions.

Generators
----------

* **_303** (mono, 303-like synth, 6 global parameters, 3 track parameters).
Totally based on the [Paul Kellet's resonant filter][4]. Accent and envmod are not implemented (yet).
Just one base signal (sawtooth). Has a period slide feature, uses a slide note and a slide length (in ticks).

* **_xi** (mono, sampler, 0 global parameters, 1 track parameter).
XI samples loader and player. Only one track and one sample per machine.
Supports envelopes, loops, can store wave data as 4-bit PCM to save some space.

* **_voice** (mono, voice synth, based on 2-formant speech synth by Frank Baumgartner).
Not finished, currently more like a PoC. Tries to be small (that's why only 2 formants).

Effects
-------

* **_filter** (mono, resonance and distortion, 3 global parameters, 0 track parameters).
Also based on the resonant filter. Adds distortion feature (can boost output volume up to 64x).

* **_delay** (stereo, cross-delay, 4 global parameters, 0 track parameters).
A stereophonic delay (X-delay, ping-pong echo, etc.). Stereo filter that ties everything together.

Samples
-------

Size-optimized ADSR with loops, can be stored as 4-bit. Mostly ripped from Quad/Theta (Wired 1998).

* bd.xi - Bass drum
* cc.xi - Crash cymbal (this is the longest one and masterfully looped)
* ch.xi - Closed Hihat
* cp.xi - Clap
* oh.xi - Open Hihat
* sd.xi - Snare drum

References
----------

####Resonant filter

See http://www.musicdsp.org/showone.php?id=29 (posted by Paul Kellett)

This filter consists of two first order low-pass filters in
series, with some of the difference between the two filter
outputs fed back to give a resonant peak.

You can use more filter stages for a steeper cutoff but the
stability criteria get more complicated if the extra stages
are within the feedback loop.

```
//set feedback amount given f and q between 0 and 1
fb = q + q/(1.0 - f);

//for each sample...
buf0 = buf0 + f * (in - buf0 + fb * (buf0 - buf1));
buf1 = buf1 + f * (buf0 - buf1);
out = buf1;
```

[1]: http://pouet.net/prod.php?which=7468
[2]: https://github.com/joric/flashbmxplay
[3]: https://github.com/joric/bmxplayjs
[4]: http://www.musicdsp.org/showone.php?id=29
