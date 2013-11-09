//_filter machine for bmxplay

#define _filter_TRACKS 0

#pragma pack(1)

//global parameters
typedef struct{
	byte param1;
	byte param2;
	byte param3;
}_filter_gpar;

//track parameters
typedef struct{
	char dummy;
}_filter_tpar;


//track info
typedef struct{
	//some useful vars for track processing
	//(...)
	char dummy;
}_filter_track;

//machine info
typedef struct{
	int tracks; //number of tracks
	_filter_gpar gpar;
	//some useful vars for machine processing
	double p1,p2,p3,buf0,buf1;
	//(...)
	//some useful vars for track processing
	//(...)
} _filter_machine;

#pragma pack()


//init procedure
void *_filter_init(const char *msd)
{
	_filter_machine* m=(_filter_machine*)bmx_alloc(sizeof(_filter_machine));
	(void)msd;

	m->buf0=0;
	m->buf1=0;

	return m;
}

int _filter_tick(_filter_machine *m, _filter_gpar *gp, _filter_tpar *tp)
{
	(void)tp;
	if (gp->param1!=0xFF)	m->p1=gp->param1/128.0;
	if (gp->param2!=0xFF)	m->p2=gp->param2/128.0;
	if (gp->param3!=0xFF)	m->p3=gp->param3/128.0;
	return 0;
}

BOOL _filter_work(_filter_machine *m, float *psamples, int numsamples, int channels)
{
	(void)channels;

	BmxResonanceFilter(psamples, numsamples, m->p1*0.99, m->p2*0.98, &m->buf0, &m->buf1);

	//distortion
	if (m->p3!=0)
	{
		int i;
		float amp = 1.0f / (float)(1.0f-m->p3);
		for (i=0; i<numsamples;i++)
		{
			float a=psamples[i];
			a *= amp; if (a>32767) a=32767; if (a<-32767) a=-32767;
			psamples[i]=a;
		}
	}
	return TRUE;
}

#pragma pack(1)
//machine info
BmxMachineHeader _filter_header={
	"_filter", //dllname
	3, //gpsize in bytes
	0, //tpsize in bytes
	1, //channels (1-mono, 2-stereo)
	(LPFINIT)&_filter_init,
	(LPFTICK)&_filter_tick,
	(LPFWORK)&_filter_work
};
#pragma pack()


