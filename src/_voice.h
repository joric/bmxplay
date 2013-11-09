//_voice machine for bmxplay

// based on two-formant speech synthesizer
// by baumgartner frank

#include <stdlib.h> //rand

#define _voice_TRACKS 1

#pragma pack(1)

//global parameters
typedef struct
{
	byte tune;
	byte cutoff;
	byte resonance;
	byte envmod;
	byte decay;
	byte accent;
} _voice_gpar;

//track parameters
typedef struct
{
	byte note;
	byte slide;
	byte endnote;
} _voice_tpar;

#define SRATE   44100
#define Ta      ( 1.0f / SRATE )

#ifndef M_PI
#define M_PI    3.141592654f
#endif

#define sqr(a)  ( (a)*(a) )
#define exp2(x)	( pow( 2.0, x * 1.442695041 ) )

typedef struct
{				// digital designed 12dB IIR resonator

	float a, b, c;		// filter vars, coeff's
	float y, z1, z2;	// (2 pole, 12 dB)

} T_BP;

typedef struct
{
	char ch;
	int f1, f2, b1, b2, a1, a2, asp, len;
} _voice_preset;

const char* str0 = "";

const char* _phrases[] = {
	"eeeh poolnaaa polnaa moia korooobotckaaaa",
	"eeeestv nei siiiitets iiii paaartcaaaaaa",
	"pooozhaleei dusha moa zaznoobutkaaaaaa",
	"maaaalodeeedskaaagoo pleeeeechaaaaaaa",
	"aaaaaaaaa",
	"iiiiiiiii",
	"ooooooooo"
};

_voice_preset _presets[] = {
	{' ', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'a', 700, 1100, 60, 90, 100, 100, 0, 7000},	//a
	{'b', 190, 760, 60, 90, 100, 100, 255, 500},	//b
	{'c', 190, 2680, 60, 150, 25, 25, 255, 500},
	{'d', 190, 2680, 60, 150, 25, 25, 255, 500},	//d
	{'e', 600, 1800, 60, 90, 100, 100, 0, 4500},	//e
	{'f', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'g', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'h', 190, 1480, 30, 45, 100, 100, 255, 1000},
	{'i', 300, 2200, 60, 90, 100, 100, 1, 5000},	//i
	{'j', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'k', 190, 1480, 30, 45, 100, 100, 255, 1000},	//k
	{'l', 460, 1480, 60, 90, 100, 100, 1, 4000},	//l
	{'m', 480, 1000, 40, 175, 100, 100, 1, 5000},	//m
	{'n', 480, 1780, 40, 300, 100, 0, 0, 3000},	//n
	{'o', 610, 880, 60, 90, 100, 100, 0, 5500},	//o
	{'p', 190, 760, 60, 90, 100, 100, 255, 500},
	{'q', 190, 1480, 30, 45, 100, 100, 255, 1000},
	{'r', 490, 1180, 60, 90, 100, 100, 1, 5000},	//r
	{'s', 2620, 13000, 200, 220, 5, 15, 255, 4000},	//s
	{'t', 190, 2680, 60, 150, 100, 100, 255, 500},	//t
	{'u', 300, 950, 60, 90, 100, 100, 1, 5000},	//u
	{'v', 280, 1420, 60, 90, 100, 100, 255, 1000},	//v
	{'w', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'x', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'y', 550, 2000, 60, 90, 0, 0, 1, 1500},
	{'z', 1720, 14000, 60, 90, 10, 10, 255, 5000},	//z
/*
{' ',550,2000,60,90,0,0,1,1500},
{'o',610,880,60,90,100,100,1,4000},
{'t',190,2680,60,150,100,100,255,500},
{'e',600,1800,60,90,100,100,0,4500},
{'k',1480,30,45,100,100,255,1000},
{'l',460,1480,60,90,100,100,1,4000},
{'o',610,880,60,90,100,100,1,4000},
{'r',490,1180,60,90,100,100,1,5000},
{'d',190,2680,60,150,25,25,255,500},                //d
{'s',2620,13000,200,220,5,15,255,4000},             //s
{'t',190,2680,60,150,50,50,255,500},                //t
{'a',700,1100,60,90,100,100,0,7000},                //a
{'s',2620,13000,200,220,5,15,255,4000},             //s
{'b',190,760,60,90,100,100,255,500},                //b
{'o',500,950,60,90,100,100,1,10000},                //o
{'t',190,2680,60,150,100,100,255,500},              //t
{'t',190,2680,60,150,100,100,255,500},              //t
{'e',600,1800,60,90,100,100,0,4500},                //e
{'k',190,1480,30,45,100,100,255,1000},              //k
{'n',480,1780,40,300,100,0,0,3000},                 //n
{'o',610,880,60,90,100,100,0,5500},                 //o
{'m',480,1000,40,175,100,100,1,5000},               //m
{'a',700,1100,60,90,100,100,1,5000},                //a
{'k',190,1480,30,45,100,100,255,1000},              //k
{'s',2620,13000,200,220,5,15,255,4000},             //s
{'i',300,2200,60,90,100,100,1,5000},                //i
{'m',480,1000,40,175,100,100,1,5000},               //m
{'u',300,950,60,90,100,100,1,5000},                 //u
{'m',480,1000,40,175,100,100,1,5000},               //m
{'v',280,1420,60,90,100,100,255,1000},              //v
{'e',600,1800,60,90,100,100,0,4000},                //e
{'l',460,1480,60,90,100,100,0,4000},                //l
{'o',610,880,60,90,100,100,0,4000},                 //o
{'s',2620,13000,200,220,5,15,255,4000},             //s
{'i',300,2200,60,90,100,100,1,4000},                //i
{'t',190,2680,60,150,50,50,255,500},                //t
{'i',300,2200,60,90,100,100,1,4000},                //i
{'u',300,950,60,90,100,100,1,7000},                 //u
{'n',480,1780,40,300,100,0,1,5000},                 //n
{'t',190,2680,60,150,50,50,255,500},                //t
{'t',190,2680,60,150,100,100,255,500},              //t
{'a',700,1100,60,90,100,100,0,7000},                //a
{'n',480,1780,40,300,100,0,0,7000},                 //n
{'z',1720,14000,60,90,10,10,255,5000},              //z
{'e',600,1800,60,90,100,100,0,7000},                //e
{'n',480,1780,40,300,100,0,0,7000},                 //n
{'?',250,1800,60,90,100,100,1,5000},                //?
{'b',190,760,60,90,200,200,255,1500},               //b
{'e',600,1800,60,90,100,100,1,5000},                //e
{'r',490,1180,60,90,100,100,1,5000},                //r
{'k',190,1480,30,45,100,100,255,2000},              //k
{'e',600,1800,60,90,100,100,0,5000},                //e
{'n',480,1780,40,300,100,0,0,3000},                 //n
{'s',2620,13000,200,220,5,15,255,4000},             //s
{'i',300,2200,60,90,100,100,1,5000},                //i
{'r',490,1180,60,90,100,100,1,4000},                //r
{'o',610,880,60,90,100,100,0,4000},                 //o
{'k',190,1480,30,45,100,100,255,1000},			    //k
*/

};

//track info
typedef struct
{
	//some useful vars for track processing
	float freq;
	int slide;
	float freq1;
	float dfreq;
	int tune;

	float a, da;
	float s, smax, dsmax;

	float f, q, df;
	float amp, damp;
	char *str;
	int strpos;

	int f1, f2, b1, b2, a1, a2, asp, len;
	int ff1, ff2, flen, bw1, bw2;
	float fm1, fm2, am1, am2, as, saw, mul;

	T_BP bp[2];

	//(...)
} _voice_track;

//machine info
typedef struct
{
	int tracks;		//number of tracks
	_voice_gpar gpar;
	_voice_tpar tpar[_voice_TRACKS];	//linked
	//some useful vars for machine processing
	float f, q;
	long pos;
	float decay;
	int tune;
	char *msg[1];
	//(...)
	//some useful vars for track processing
	_voice_track tv[_voice_TRACKS];
} _voice_machine;



void bp_init(T_BP * bp, float f, float bw)
{
	float r = (float)exp2(-M_PI * bw * Ta);
	bp->c = -r * r;
	bp->b = r * 2.0f * cos(2.0f * M_PI * f * Ta);
	bp->a = 1.0f - bp->b - bp->c;
}

void bp_iir(T_BP * bp, float in)	// 12 dB bi-quad
{
	bp->y = bp->a * in + bp->b * bp->z1 + bp->c * bp->z2;
	bp->z2 = bp->z1;
	bp->z1 = bp->y;
}


#pragma pack()

//init procedure
void *_voice_init(char *msd)
{
	(void)msd;

	_voice_machine *m = (_voice_machine *) bmx_alloc(sizeof(_voice_machine));
	_voice_track *t = &m->tv[0];

	m->pos = 0;
	t->freq = 130.815f; //C-4
	t->dfreq = 0;
	t->slide = 0;
	m->tune = 0;

	t->fm1 = 0.0;
	t->fm2 = 0.0;
	t->bw1 = 0;
	t->bw2 = 0;
	t->am1 = 0.0;
	t->am2 = 0.0;
	t->as = 0.0;

	t->saw = 0.0;

	t->f1 = 0;
	t->f2 = 0;
	t->b1 = 0;
	t->b2 = 0;
	t->a1 = 0;
	t->a2 = 0;
	t->asp = 0;
	t->len = 0;

	t->s = -1;
	t->strpos = -1;
	t->str = (char*)str0;

	memset(t->bp, 0, sizeof(T_BP)*2);

	return m;
}

int _voice_tick(_voice_machine * m, _voice_gpar *gp, _voice_tpar * tp)
{
	int track = 0;
	(void)gp;
	_voice_track *t = &m->tv[0];

	if (tp->slide != 0xFF)
	{
		int c = sizeof(_phrases) / sizeof(_phrases[0]);
		int i = tp->slide;

		t->strpos = 0;
		t->s = 0;

		if (i < c)
			t->str = (char*)_phrases[i];
		else
			t->str = (char*)str0;
	}

	if (tp->note != 0)
	{
		t->freq = getfreq((int)tp[track].note + m->tune);
	}

	return 0;
}

_voice_preset *get_p(char s)
{
	int i;
	int c = sizeof(_presets) / sizeof(_presets[0]);

	_voice_preset *p;

	for (i = 0; i < c; i++)
	{
		p = &_presets[i];
		if (p->ch == s)
			return p;
	}
	return &_presets[0];
}

BOOL _voice_work(_voice_machine * m, float *psamples, int numsamples, int channels)
{
	int i;
	int track = 0;
	(void)channels;

	_voice_track *t = &m->tv[track];

	for (i = 0; i < numsamples; i++)
	{
		if (t->s == 0 && t->strpos >= 0 && t->strpos < (int)strlen(t->str))	//load phoneme
		{
			_voice_preset *p = get_p(t->str[t->strpos]);

			t->f1 = p->f1;
			t->f2 = p->f2;
			t->b1 = p->b1;
			t->b2 = p->b2;
			t->a1 = p->a1;
			t->a2 = p->a2;
			t->asp = p->asp;
			t->len = p->len;

			t->fm1 = 0.0;
			t->fm2 = 0.0;
			t->bw1 = 50;
			t->bw2 = 150;
			t->am1 = 0.0;
			t->am2 = 0.0;
			t->as = 0.0;
			t->saw = 0.0;

			t->ff1 = t->f1 * 255 / 3500;
			t->ff2 = t->f2 * 255 / 3500;
			t->flen = t->len * 2550 / 65536;

			t->strpos++;
		}

		if (t->strpos >= 0 && t->s >= 0)
		{
			float out;
			double src;

			if (t->asp)
				t->mul = 1.0;
			else
				t->mul = 1.0 / 512.0;

			t->fm1 += (t->f1 - t->fm1) * t->mul;
			t->fm2 += (t->f2 - t->fm2) * t->mul;
			t->bw1 += (int)((t->b1 - t->bw1) * t->mul);
			t->bw2 += (int)((t->b2 - t->bw2) * t->mul);
			t->am1 += (t->a1 - t->am1) * t->mul;
			t->am2 += (t->a2 - t->am2) * t->mul;
			t->as += (t->asp - t->as) * t->mul;

			bp_init(&t->bp[0], (float)t->fm1, (float)t->bw1);
			bp_init(&t->bp[1], (float)t->fm2, (float)t->bw2);

			t->saw += t->freq / 44100.0f - (int) t->saw;

			src = (t->saw - 0.5) * (1.0 - t->as / 256.0) + t->as / 256.0 * ((rand() & 255) / 256.0 - 0.5);

			bp_iir(&t->bp[0], (float)src * t->am1);
			bp_iir(&t->bp[1], (float)src * t->am2);

			out = t->bp[0].y + t->bp[1].y;

			psamples[i] = out * 127.0f;

			t->s++;

			if (t->s >= t->len)
			{
				t->s = 0;
				if (t->strpos >= (int)strlen(t->str))
				{
					t->s = -1;
					t->strpos = -1;
				}
			}
		}
		else
			return FALSE;
	}
	return TRUE;
}

//machine info
BmxMachineHeader _voice_header = {
	"_voice",		//dllname
	sizeof(_voice_gpar),	//gpsize in bytes
	sizeof(_voice_tpar),	//tpsize in bytes
	1,			//channels (1-mono, 2-stereo)
	(LPFINIT)&_voice_init,
	(LPFTICK)&_voice_tick,
	(LPFWORK)&_voice_work
};

#pragma pack()
