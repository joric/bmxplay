# Bmxplay

Tiny player (up to 3 kb) for Jescola's Buzz by Joric^Proxium.
First used in 64k-intro [Yo-08][1] (2002).
Completely written in plain C (mostly in october 2001), considering a _future possibility_
of porting everything to assembly. Uses its own C++ classes implementation in plain C.
Used to patch virtual methods table in order to comply with various versions of Buzz.
If you want conventional or "modern" OOP, check out [flashbmxplay][2] or [bmxplayjs][3].

## Demo

https://joric.github.io/bmxplayjs

## License

Player is public domain. MachineInterface.h (written by Oskari) and (therefore) all dlls are for
non-commercial use. Note that player neither links them nor uses their API or header (they are
only for composing music in Buzz), so you can use the player in a commercial app
(or a keygen), without legal restrictions.

## Generators

* **_303** (mono, 303-like synth, 6 global parameters, 3 track parameters).
Totally based on the [Paul Kellet's resonant filter][4]. Accent and envmod are not implemented (yet).
Just one base signal (sawtooth). Has a period slide feature, uses a slide note and a slide length (in ticks).

* **_xi** (mono, sampler, 0 global parameters, 1 track parameter).
XI samples loader and player. Only one track and one sample per machine.
Supports envelopes, loops, can store wave data as 4-bit PCM to save some space.

* **_voice** (mono, voice synth, based on 2-formant speech synth by [Frank Baumgartner][5]).
Not finished, currently more like a PoC. Tries to be small (that's why only 2 formants).

## Effects

* **_filter** (mono, resonance and distortion, 3 global parameters, 0 track parameters).
Also based on the resonant filter. Adds distortion feature (can boost output volume up to 64x).

* **_delay** (stereo, cross-delay, 4 global parameters, 0 track parameters).
A stereophonic delay (X-delay, ping-pong echo, etc.). Stereo filter that ties everything together.

## Samples

Size-optimized ADSR with loops, can be stored as 4-bit (2 times smaller). Mostly ripped from Quad/Theta (Wired 1998).

* bd.xi (5344 bytes) - Bass drum
* cc.xi (6672 bytes) - Crash cymbal (this is the longest one and masterfully looped)
* ch.xi (2323 bytes) - Closed Hihat
* cp.xi (1924 bytes) - Clap
* oh.xi (3950 bytes) - Open Hihat
* sd.xi (3737 bytes) - Snare drum

## References

### Resonant filter

See http://www.musicdsp.org/showone.php?id=29 (posted by Paul Kellett)

This filter consists of two first order low-pass filters in
series, with some of the difference between the two filter
outputs fed back to give a resonant peak.

You can use more filter stages for a steeper cutoff but the
stability criteria get more complicated if the extra stages
are within the feedback loop.

```c
//set feedback amount given f and q between 0 and 1
fb = q + q/(1.0 - f);

//for each sample...
buf0 = buf0 + f * (in - buf0 + fb * (buf0 - buf1));
buf1 = buf1 + f * (buf0 - buf1);
out = buf1;
```

### Speech synthesizer

Based on the 2-formant speech synth from the Dialogos 2001 [4K intro](https://demozoo.org/productions/39656/) by [Frank Baumgartner][5], published in Hugi magazine (example below).
Older versions included 6-formant Klatt synth ([klatt-3.04](http://bmxplay.sourceforge.net/files/klatt.3.04.tar.gz)) used by Stephen Hawking since mid-80s,
but it was considered too large.

```c
// two-formant speech synthesizer
// (c) 2000-2001 by baumgartner frank
// example by joric for the bmxplay project

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define M_PI 3.141592654f
#define Ta ( 1.0f / 44100 )
#define exp2(x) ( pow( 2.0, x * 1.442695041 ) )

typedef struct {		// digital designed 12dB IIR resonator
	float  a, b, c;		// filter vars, coeff's
	float  y, z1, z2;	// (2 pole, 12 dB)
} T_BP;

T_BP bp[ 2 ];

void bp_init( T_BP &bp, float f, float bw ) {
	float r = exp2( -M_PI*bw*Ta );
	bp.c = - r*r;
	bp.b = r * 2.0f * cos( 2.0f*M_PI*f*Ta );
	bp.a = 1.0f - bp.b - bp.c;
}

void bp_iir( T_BP &bp, float in ) {	// 12 dB bi-quad
	bp.y = bp.a*in + bp.b*bp.z1 + bp.c*bp.z2;
	bp.z2 = bp.z1;
	bp.z1 = bp.y;
}

typedef struct {
	char ch;
	int f1, f2, b1, b2, a1, a2, asp, len;
} phoneme;

phoneme phonemes[] = {
	{' ', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'a', 700, 1100, 60, 90, 100, 100, 0, 7000},
	{'b', 190, 760, 60, 90, 100, 100, 255, 500},
	{'c', 190, 2680, 60, 150, 25, 25, 255, 500},
	{'d', 190, 2680, 60, 150, 25, 25, 255, 500},
	{'e', 600, 1800, 60, 90, 100, 100, 0, 4500},
	{'f', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'g', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'h', 190, 1480, 30, 45, 100, 100, 255, 1000},
	{'i', 300, 2200, 60, 90, 100, 100, 1, 5000},
	{'j', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'k', 190, 1480, 30, 45, 100, 100, 255, 1000},
	{'l', 460, 1480, 60, 90, 100, 100, 1, 4000},
	{'m', 480, 1000, 40, 175, 100, 100, 1, 5000},
	{'n', 480, 1780, 40, 300, 100, 0, 0, 3000},
	{'o', 610, 880, 60, 90, 100, 100, 0, 5500},
	{'p', 190, 760, 60, 90, 100, 100, 255, 500},
	{'q', 190, 1480, 30, 45, 100, 100, 255, 1000},
	{'r', 490, 1180, 60, 90, 100, 100, 1, 5000},
	{'s', 2620, 13000, 200, 220, 5, 15, 255, 4000},
	{'t', 190, 2680, 60, 150, 100, 100, 255, 500},
	{'u', 300, 950, 60, 90, 100, 100, 1, 5000},
	{'v', 280, 1420, 60, 90, 100, 100, 255, 1000},
	{'w', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'x', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'y', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'z', 1720, 14000, 60, 90, 10, 10, 255, 5000}
};

int main() {
	printf("Writing out.wav...\n");
	memset( &bp, 0, sizeof bp );
	float fm1 = .0f, fm2 = .0f, bw1 = .0f, bw2 = .0f, am1 = .0f, am2 = .0f, as = .0f, saw = .0f, mul;
	FILE *fp = fopen("out.wav", "wb");
	int wav[] = {1179011410, 73284, 1163280727, 544501094, 16, 65537, 44100, 44100, 524289, 1635017060};
	fwrite(wav, 1, sizeof(wav), fp);

	for (const char *p = "teklords"; *p; p++) {
		phoneme *f = &phonemes[ *p>='a' && *p<='z' ? *p+1-'a' : 0 ];
		bw1 = 50.0f; bw2 = 150.0f;

		// write wave data
		for ( int i=0; i<f->len; i++ ) {
			mul = f->asp ? 1.0f : 1.0f / 512.0f;
			fm1 += ( f->f1 - fm1 ) * mul;
			fm2 += ( f->f2 - fm2 ) * mul;
			bw1 += ( f->b1 - bw1 ) * mul;
			bw2 += ( f->b2 - bw2 ) * mul;
			am1 += ( f->a1 - am1 ) * mul;
			am2 += ( f->a2 - am2 ) * mul;
			as  += ( f->asp - as ) * mul;
			bp_init( bp[0], fm1, bw1 );
			bp_init( bp[1], fm2, bw2 );
			saw += 90.0f/44100.0f - (int)saw;
			double src = (saw-0.5f) * (1.0f-as/256.0f) + as/256.0f * ((rand()&255)/256.0f - 0.5f);
			bp_iir( bp[0], src * am1 );
			bp_iir( bp[1], src * am2 );
			float out = bp[0].y + bp[1].y;
			char s = out * 0.3f;
			fputc((s+255)/2, fp);
		}
	}
	fclose(fp);
}
```

[1]: http://pouet.net/prod.php?which=7468
[2]: https://github.com/joric/flashbmxplay
[3]: https://github.com/joric/bmxplayjs
[4]: http://www.musicdsp.org/showone.php?id=29
[5]: http://www.active-web.cc/html/research/
