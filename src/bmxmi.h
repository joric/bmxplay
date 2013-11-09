//BmxPlay project (c) 2001-2012 Joric^Proxium

//bmxplay machine interface

///////////////////////////////////////////
int BmxPosInTick = 0;
int BmxCurrentTick = 0;
int BmxTicksPerSequence = 0;
int BmxSamplesPerTick = 0;
int BmxSamplesPerSecond = 44100;
///////////////////////////////////////////

#define BMXWAVES 32
#define ENVELOPES 4
#define ENVPOINTS 32

typedef void *(*LPFINIT) (char *);
typedef int (*LPFTICK) (void *, char *, char *);
typedef BOOL (*LPFWORK) (void *, float *, int, int);

#pragma pack(1)

typedef struct
{
	WORD x, y;
} EnvPoints;

typedef struct
{
	WORD x, y;
	byte flag;
} BmxEnvPoints;

typedef struct
{
	WORD attack;
	WORD sustain;
	WORD decay;
	WORD release;
	byte ADSRsubdivide;
	byte ADSRflags;
	WORD points;
	BmxEnvPoints *p;
} BmxEnvelope;

typedef struct
{
	WORD index;
	char *filename;
	char *name;
	float volume;
	byte flags;
	WORD envelopes;
	BmxEnvelope e[ENVELOPES];
	byte levels;
	DWORD samples;
	DWORD loopbeg;
	DWORD loopend;
	DWORD sps;
	byte note;
	byte format;
	DWORD bytes;
	char *data;
} BmxWaves;

typedef struct
{
	const char *dllname;	//dllname
	long gpsize;		//gpsize in bytes
	long tpsize;		//tpsize in bytes
	int nChannels;		//channels (1-mono, 2-stereo)
	LPFINIT init;
	LPFTICK tick;
	LPFWORK work;
} BmxMachineHeader;

#pragma pack()

//BmxWaves *bmxwaves;

//some useful code for machines
float getfreq(byte note)
{
	if (note != 0xFF && note > 0)
	{
		int l_Note = ((note >> 4) * 12) + (note & 0x0f) - 70;
		return (float) (440.0 * pow(2.0, ((float) l_Note) / 12.0));

	}
	else
		return 0;
}

void BmxResonanceFilter(float *psamples, int numsamples, double f, double q, double *b0, double *b1)
{
	int i;
	double in, fb, buf0, buf1;

	buf0 = *b0;
	buf1 = *b1;

	fb = q + q / (1.0 - f);

	for (i = 0; i < numsamples; i++)
	{
		in = psamples[i];

		//for each sample...
		buf0 = buf0 + f * (in - buf0 + fb * (buf0 - buf1));
		buf1 = buf1 + f * (buf0 - buf1);

		psamples[i] = (float) buf1;
	}

	*b0 = buf0;
	*b1 = buf1;
}

void *bmx_alloc(int bytes)
{
	return malloc(bytes);
}

#define bmx_free(p) { if (p) free(p); p=NULL; }
