//BmxPlay project (c) 2001-2012 Joric^Proxium

//the bmxplay engine

#define BMXPLAY_VERSION "0.4.6"

float BmxPerfomance;		// required timeslice in %

#ifdef _WIN32
#include "waveout_win32.h"
#else
#include <stdint.h>
#define DWORD uint32_t
#define WORD uint16_t
#define byte uint8_t
#define BOOL char
#define TRUE 1
#define FALSE 0
#include "waveout_linux.h"
#endif

int BmxBufferSize = BUFSIZE;

#ifdef _MSC_VER
#pragma warning (disable: 4200 4018 4101 4244)
#endif

double const PI = 3.14159265358979323846;

#include "bmxmi.h"
#include "bmxmlist.h"

#define BMX_ENTRIES 31
#define BMX_MACHINES 64
#define BMX_PATTERNS 256
#define BMX_TRACKS 256
#define BMX_SEQUENCES 8192
#define BMX_MACHINE_ATTRS 64

#pragma pack(1)

typedef struct
{
	DWORD name;
	DWORD offset;
	DWORD size;
} BmxDirEntry;

typedef struct
{
	char title[4];
	DWORD sections;
	BmxDirEntry entry[BMX_ENTRIES];
} BmxHeader;

typedef struct
{
	WORD src;
	WORD dst;
	WORD amp;
	WORD pan;
} BmxConnection;

typedef struct
{
	WORD connections;
	BmxConnection c[];
} BmxConnectionsList;

typedef struct
{
	char *name;
	DWORD value;
} BmxMachineAttr;

typedef struct
{
	char *name;
	char type;
	char *dllname;
	DWORD datalength;
	char *data;
	WORD attrs;
	BmxMachineAttr attr[BMX_MACHINE_ATTRS];
	char *gpar;		// globals - e.g. for master: vol, bpm, tpb: w w b
	char *tpar;		// track vars - e.g. amp, pan, notes and so on
	WORD channels;
	WORD sources;
	WORD tracks;
	WORD firstpattern;
	WORD patterns;
	//these seems to be extracted from dll
	WORD gpsize;		//gpar size in bytes
	WORD tpsize;		//tpar size in bytes
	BmxMachineHeader *mh;
	//pointer to machine instance
	void *me;
	//vars for mixing
	int scount;
	float buf[BUFSIZE][2];
	BOOL active;
} BmxMachine;
	
typedef struct
{
	WORD machines;
	BmxMachine m[BMX_MACHINES];
} BmxMachinesList;

typedef struct
{
	char *name;
	WORD rows;
	char *gdata;
	char *tdata[BMX_PATTERNS];
	char *sdata[BMX_TRACKS];
} BmxPattern;

typedef struct
{
	WORD machine;
	DWORD events;
	byte psize;
	byte esize;
	WORD event;
	WORD row;
	char *data;
} BmxSequence;

typedef struct
{
	DWORD songsize;
	DWORD startloop;
	DWORD endloop;
	WORD sequences;
	BmxSequence s[BMX_SEQUENCES];
} BmxSequencesList;


typedef struct
{
	BmxConnectionsList *con;
	BmxMachinesList mach;
	BmxPattern p[BMX_PATTERNS];
	BmxSequencesList seq;
	//WORD waves;
	//BmxWaves w[BMXWAVES];
} Bmx;

#pragma pack()

char *bmxp;

char *geta()
{
	char *a = bmxp;
	while (*bmxp != 0)
		bmxp++;
	bmxp++;
	return a;
}

byte getByte()
{
	bmxp++;
	return *(byte *) (bmxp - 1);
}

WORD getWord()
{
	bmxp += 2;
	return *(WORD *) (bmxp - 2);
}

DWORD getDword()
{
	bmxp += 4;
	return *(DWORD *) (bmxp - 4);
}

float getFlt()
{
	bmxp += 4;
	return *(float *) (bmxp - 4);
}
char *getStr(long bytes)
{
	bmxp += bytes;
	return bmxp - bytes;
}

Bmx bmx;

int bmxgears;

int BmxGetSamplesPerTick()
{
	WORD BPM = *(WORD *) (bmx.mach.m[0].gpar + 2);
	WORD TPB = *(WORD *) (bmx.mach.m[0].gpar + 4);
	return (int) ((60 * 44100) / (BPM * TPB));
}

enum
{
	ERR_WRONG_HEADER = 1,
	ERR_WRONG_DATA,
	ERR_UNKNOWN_MACHINE
};

#define FOURCC(a,b,c,d) ( (((d)<<24) | ((c)<<16) | ((b)<<8) | (a)) )

int BmxLoad(char *data)
{
	int i, j, k, t, h, n;

	BmxHeader *header;
	BmxMachine *m;
	BmxPattern *p;
	BmxSequence *s;

	header = (BmxHeader *) data;

	if (*(DWORD *) header != FOURCC('B','u','z','z'))
		return ERR_WRONG_HEADER;

	if (header->sections > BMX_ENTRIES)
		return ERR_WRONG_DATA;

	//we have to map some structures to bmx address space.

	for (h = 0; h < (int) header->sections; h++)
	{
		bmxp = data + header->entry[h].offset;

		switch (header->entry[h].name)
		{
		case FOURCC('M','A','C','H'):
			bmx.mach.machines = getWord();

			if (bmx.mach.machines > BMX_MACHINES)
				return ERR_WRONG_DATA;

			for (i = 0; i < bmx.mach.machines; i++)
			{
				BOOL found = FALSE;

				m = &bmx.mach.m[i];

				m->name = geta();
				m->type = getByte();

				if (m->type != 0)
					m->dllname = geta();
				else
					m->dllname = (char*)"Master";

				bmxgears = sizeof(BmxGears) / sizeof(BmxGears[0]);

				for (k = 0; k < bmxgears; k++)
				{
					if (strcmp(m->dllname, BmxGears[k]->dllname) == 0)
					{
						m->gpsize = (WORD) BmxGears[k]->gpsize;
						m->tpsize = (WORD) BmxGears[k]->tpsize;
						m->channels = BmxGears[k]->nChannels;
						found = TRUE;
					}
				}

				if (m->type == 0)
				{
					m->gpsize = 5;
					m->tpsize = 0;
					m->channels = 2;
					found = TRUE;
				}

				if (!found)
					return ERR_UNKNOWN_MACHINE;

				m->sources = 0;
				getStr(8);	//coordinates for machine view (2 x float)
				m->datalength = getDword();
				m->data = getStr(m->datalength);
				m->attrs = getWord();

				for (k = 0; k < (int) m->attrs; k++)
				{
					m->attr[k].name = geta();
					m->attr[k].value = getDword();
				}

				m->gpar = getStr(m->gpsize);
				m->tracks = getWord();
				m->tpar = getStr(m->tracks * m->tpsize);
			}
			break;

		case FOURCC('C','O','N','N'):

			bmx.con = (BmxConnectionsList *) bmxp;

			for (i = 0; i < bmx.mach.machines; i++)
			{
				m = &bmx.mach.m[i];
				for (k = 0; k < bmx.con->connections; k++)
					if (bmx.con->c[k].dst == i)
						m->sources++;
			}

			break;

		case FOURCC('P','A','T','T'):

			n = 0;
			for (i = 0; i < bmx.mach.machines; i++)
			{
				m = &bmx.mach.m[i];

				m->firstpattern = n;
				m->patterns = getWord();
				m->tracks = getWord();
				
				if (m->patterns > BMX_PATTERNS || m->tracks > BMX_TRACKS)
					return ERR_WRONG_DATA;

				for (k = 0; k < bmx.mach.m[i].patterns; k++)
				{
					p = &bmx.p[n];
					p->name = geta();
					p->rows = getWord();

					for (t = 0; t < m->sources; t++)
					{
						getWord();
						p->sdata[t] = getStr(p->rows * /*m->channels */ 2 * 2);
					}

					p->gdata = getStr(p->rows * m->gpsize);
					for (t = 0; t < m->tracks; t++)
						p->tdata[t] = getStr(p->rows * m->tpsize);
					n++;	//next pattern
				}
			}

			break;

		case FOURCC('S','E','Q','U'):

			bmx.seq.songsize = getDword();
			bmx.seq.startloop = getDword();
			bmx.seq.endloop = getDword();
			bmx.seq.sequences = getWord();

			if (bmx.seq.sequences > BMX_SEQUENCES)
				return ERR_WRONG_DATA;

			for (i = 0; i < bmx.seq.sequences; i++)
			{
				s = &bmx.seq.s[i];
				s->machine = getWord();
				s->events = getDword();
				if (s->events != 0)
				{
					s->psize = getByte();
					s->esize = getByte();
					s->event = 0;
					s->row = 0;
					s->data = getStr(s->events * (s->esize + s->psize));
				}
			}
			break;
		}
	}

	//init machines
	for (i = 1; i < bmx.mach.machines; i++)
	{
		m = &bmx.mach.m[i];

		for (j = 0; j < bmxgears; j++)
			if (strcmp(m->dllname, BmxGears[j]->dllname) == 0)
				m->mh = BmxGears[j];

		//init
		m->me = ((LPFINIT) (m->mh->init)) (m->data);
		//make a tick to load initial data
		((LPFTICK) (m->mh->tick)) (m->me, m->gpar, m->tpar);

		//enable filters by default
		if (m->type == 2)
			m->active = TRUE;
	}

	//init variables
	BmxCurrentTick = 0;	//bmx.seq.startloop;
	BmxTicksPerSequence = bmx.seq.songsize;
	BmxSamplesPerTick = BmxGetSamplesPerTick();

	return 0;
}

static void BmxQuantize(float *pin, int *piout, int c)
{
	#define NORM(n) ((n<-32767) ? -32767 : ((n>32767) ? 32767 : n));

	int i;

	short *w = (short*)piout;

	for (i=0; i<c*2; i++) //both channels
	{	
		float fa = *pin++;
		*w++ = (short)NORM(fa);
	}
}

int BmxSmartMix(int samples, short *dest)
{
	BmxMachine *m, *m1;
	float *s, *d;
	int i, k;
	int machine = 0;
	int machines = bmx.mach.machines;

	//0 - thru, 0x4000 = -80db
	float mastervolume = (1.0 - (*(WORD *) (bmx.mach.m[0].gpar) / (float) 0x4000));

	for (i = 0; i < machines; i++)
	{
		m = &bmx.mach.m[i];
		m->scount = m->sources;

		//clearing buffers <- this one seems to be fukin' bottleneck

		//types: 0-master 1-generator 2-filter
		//it's assumed that generator must clear buffer tail himself
		if (m->channels != 2)
			memset(m->buf, 0, samples * sizeof(float));
		else
			memset(m->buf, 0, samples * sizeof(float) * 2);
	}

	// m->scount is number of sources of current machine
	// if scount = 0 it mean that whe rendered all sources and can render the actual machine
	// if scount < 1 all sources and actial machine have been processed


	//until all sources of Master won't be rendered
	while (bmx.mach.m[0].scount != 0)
	{
		if (bmx.mach.m[machine].scount != 0 || bmx.mach.m[machine].scount < 0)
			machine++;	//next if cannot evaluate yet, or machine has been processed
		else
		{
			m = &bmx.mach.m[machine];
			if (m->active)
				((LPFWORK) (m->mh->work)) (m->me, (float *) (m->buf), samples, m->channels);

			for (k = 0; k < bmx.con->connections; k++)
			{
				if (bmx.con->c[k].src == machine)
				{
					float lamp, ramp;
					int c = samples;
					float amp = (float) bmx.con->c[k].amp / (float) 0x4000;
					float rpan = (float) bmx.con->c[k].pan / (float) 0x8000;
					float lpan = 1 - rpan;

					m1 = &bmx.mach.m[bmx.con->c[k].dst];
					s = (float *) (m->buf);
					d = (float *) (m1->buf);

					//copy source to destination with corresponding amplitude and panning

					if (m1->type == 0)
						amp *= mastervolume; //master

					lamp = amp * lpan;
					ramp = amp * rpan;

					if (m->channels == 1 && m1->channels == 1)
					{
						do
						{
							*d += *s * amp;
							d++;
							s++;
						}
						while (--c);
					}

					else if (m->channels == 1 && m1->channels == 2)
					{

						do
						{
							*d += *s * lamp;
							d++;
							*d += *s * ramp;
							d++;
							s++;
						}
						while (--c);
					}

					else if (m->channels == 2 && m1->channels == 2)
					{
						do
						{
							*d += *s * lamp;
							d++;
							s++;
							*d += *s * ramp;
							d++;
							s++;
						}
						while (--c);
					}
					bmx.mach.m[bmx.con->c[k].dst].scount--;
				}
			}

			bmx.mach.m[machine].scount--;

			machine = 0;
		}
	}

	// writting to waveout
	BmxQuantize((float *) bmx.mach.m[0].buf, (int *) dest, samples);

	return 0;
}


int BmxTick()
{
	unsigned int j, k, t, r;

	char *sp;

	BmxMachine *m;
	BmxPattern *p;
	BmxSequence *s;

	WORD event;
	WORD pos;
	char *data;

	int tick = BmxCurrentTick;

	//play tick
	for (j = 0; j < bmx.seq.sequences; j++)
	{
		s = &bmx.seq.s[j];
		m = &bmx.mach.m[s->machine];
		m->active = TRUE;

		data = s->data;
		for (k = 0; k < s->events; k++)
		{
			if (s->psize != 2)
				pos = *(byte *) data;
			else
				pos = *(WORD *) data;
			data += s->psize;
			if (s->esize != 2)
				event = *(byte *) data;
			else
				event = *(WORD *) data;
			data += s->esize;
			if (pos == tick)
			{
				s->event = event;
				s->row = 0;
			}
		}

		// <mute>'s <break>'s <thru>'s treated the same way
		// Only patterns supported.

		if (s->events != 0 && s->event >= 0x10)
		{
			WORD amp, pan;

			int pat = s->event - 0x10;

			p = &bmx.p[pat + m->firstpattern];
			r = s->row;

			///////////////////////////////////////////////////
			if (s->machine >= 1)
			{
				((LPFTICK) (m->mh->tick)) (m->me, p->gdata + r * m->gpsize, p->tdata[0] + r * m->tpsize);
			}

			///////////////////////////////////////////////////

			s->row++;
			if (s->row >= p->rows)
			{
				s->row = 0;
				s->event = 0;
			}

			t = 0;
			for (k = 0; k < bmx.con->connections; k++)
			{
				if (bmx.con->c[k].dst == s->machine)
				{
					sp = p->sdata[t] + r * 2 * 2;

					amp = *(WORD *) (sp + 0);
					pan = *(WORD *) (sp + 2);

					if (amp != 0xffff)
						bmx.con->c[k].amp = amp;
					if (pan != 0xffff)
						bmx.con->c[k].pan = pan;

					t++;
				}
			}

		}

		else
			s->row = 0;
	}

	BmxCurrentTick++;

	if (BmxCurrentTick >= (int) bmx.seq.endloop)
		BmxCurrentTick = bmx.seq.startloop;
	BmxSamplesPerTick = BmxGetSamplesPerTick();

	return 0;
}

int BmxWorkBuffer(int samples, short *dest)
{
	int portion = 0;
	int count = samples;
	int maxsize;

	while (count != 0)
	{
		if (BmxPosInTick == 0)
			BmxTick();
		maxsize = BmxSamplesPerTick - BmxPosInTick;
		portion = count;
		if (portion > BmxBufferSize)
			portion = BmxBufferSize;
		if (portion > maxsize)
			portion = maxsize;
		BmxPosInTick += portion;
		if (BmxPosInTick == BmxSamplesPerTick)
			BmxPosInTick = 0;
		BmxSmartMix(portion, dest);
		dest += portion * 2;
		count -= portion;
	}
	return 0;
}
