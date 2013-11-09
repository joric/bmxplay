//_303 machine for bmxplay

#define _303_TRACKS 1

#pragma pack(1)

//global parameters
typedef struct{
	byte tune;
	byte cutoff;
	byte resonance;
	byte envmod;
	byte decay;
	byte accent;
}_303_gpar;

//track parameters
typedef struct{
	byte note;
	byte slide;
	byte endnote;
}_303_tpar;

//track info
typedef struct{
	//some useful vars for track processing
	float freq;
	int slide;
	float freq1;
	float dfreq;
	int tune;

	float a,da;
	float s,smax,dsmax;

	float f,q,df,buf0,buf1;
	float amp,damp;
	//(...)
}_303_track;

//machine info
typedef struct{
	int tracks; //number of tracks
	_303_gpar gpar; _303_tpar tpar[_303_TRACKS]; //linked
	//some useful vars for machine processing
	float f,q;
	long pos;
	float decay;
	int tune;
	//(...)
	//some useful vars for track processing
	_303_track tv[_303_TRACKS];
}_303_machine;

#pragma pack()

//init procedure
void * _303_init(char *msd)
{
	_303_machine* m=(_303_machine*)bmx_alloc(sizeof(_303_machine));
	_303_track *t=&m->tv[0];
	(void)msd;

	t->buf0=0;
	t->buf1=0;
	m->pos=0;
	t->freq=0;
	t->dfreq=0;
	t->slide=0;
	m->tune=0;
	return m;
}

int _303_tick(_303_machine *m, _303_gpar *gp, _303_tpar *tp)
{
	int track=0;
	_303_track *t=&m->tv[0];

	if (gp->tune!=0xFF) m->tune = (int)gp->tune-0x40;

	if (gp->decay!=0xFF)  m->decay=gp->decay/128.0f;

	if (tp->note!=0)
	{
		t->freq = getfreq((int)tp[track].note+m->tune);

		if (t->freq!=0)
		{
			//init saw generator
			t->s=0;
			t->a=-32767.0;

			t->smax=(float)BmxSamplesPerSecond/t->freq; //period
			t->dsmax=0;

			t->amp=1; t->damp=-(1.0f/(float)BmxSamplesPerTick*m->decay);
			t->dfreq=0;
			t->slide=1;
			t->da=65536.0f/t->smax;

			t->f=m->f;

			t->q=m->q;
			t->buf0=0;
			t->buf1=0;

			t->df = t->damp * t->f;
		}
	}

	if (tp->slide!=0xFF)  t->slide=tp->slide;

	if (tp->endnote!=0)
	{
		t->freq1 = getfreq((int)tp[track].endnote+m->tune);

		if (t->freq!=0 && t->slide!=0)
		{
			float smax1=(float)BmxSamplesPerSecond/t->freq1;
			float ispt=1.0f/(float)BmxSamplesPerTick/t->slide;
			t->dsmax=(smax1 - t->smax)*ispt;
			t->damp=-ispt;
			t->df = t->damp * t->f;
		}
	}

	if (gp->cutoff!=0xFF)	 m->f=gp->cutoff/128.0f*0.98f+0.1f;
	if (gp->resonance!=0xFF) m->q=gp->resonance/128.0f*0.88f;

	return 0;
}

BOOL _303_work(_303_machine *m, float *psamples, int numsamples, int channels)
{
	int i;
	float in,fb;
	int track=0;
	_303_track *t=&m->tv[track];
	(void)channels;

	if (t->freq!=0)
	{
		for (i=0; i<numsamples; i++)
		{
			if (t->amp>0)
			{
				in=t->a * t->amp;

				fb = t->q + t->q/(1.0f - t->f);

				//for each sample...
				t->buf0 = t->buf0 + t->f * (in - t->buf0 + fb * (t->buf0 - t->buf1));
				t->buf1 = t->buf1 + t->f * (t->buf0 - t->buf1);

				t->f += t->df;

				psamples[i] = t->buf1;

				//calculating saw
				t->smax+=t->dsmax;// period slide
				t->s++; if (t->s >= t->smax) { t->s=0; t->a=-32767.0; };
				t->a+=t->da;
				t->amp+=t->damp;
			}
			else psamples[i]=0;
		}

		return TRUE;

	} else
		return FALSE;

}

//machine info
BmxMachineHeader _303_header={
	"_303", //dllname
	sizeof(_303_gpar), //gpsize in bytes
	sizeof(_303_tpar), //tpsize in bytes
	1, //channels (1-mono, 2-stereo)
	(LPFINIT)&_303_init,
	(LPFTICK)&_303_tick,
	(LPFWORK)&_303_work,
};
#pragma pack()
