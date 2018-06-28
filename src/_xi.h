//_xi machine for bmxplay

#define _xi_TRACKS 8
#define _xi_NAMESIZE 16

#pragma pack(1)

//global parameters
typedef struct{
	char dummy;
}_xi_gpar;

//track parameters
typedef struct{
	byte note;
}_xi_tpar;

//track info
typedef struct{
	//some useful vars for track processing
	int sample;
	float freq;
	float basefreq;
	int sn,dsn;
	float a;
	float da;
	float tick,last_tick;
	int env_tick;
	int env_pos;
	int env_index;
	BOOL play;
	float tps; //ticks per sample (<1)
	//(...)
}_xi_track;

typedef struct{
	WORD x,y;
}_xi_envpoints;

typedef struct{
	int samplelength;
	int loopstart;
	int looplength;
	char volpts; //volume envelope points
	_xi_envpoints volenv[12];
	byte volflg; // (byte) Volume type: bit 0: On; 1: Sustain; 2: Loop
	byte type; // Bit 0-1: 0 = No loop, 1 = Forward loop, 2 = Ping-pong; bit 4: 16-bit sample
	byte compression; // 0 - 8 bit uncompressed; 1 - 4-bit
	char relnote; // Relative note number (signed byte)
	char finetune; // Finetune (signed byte -128..+127)
	unsigned char *wave;
	unsigned char *stream;
}_xi_sample;

//machine info
typedef struct{
	int tracks; //number of tracks
	_xi_gpar gpar; _xi_tpar tpar[_xi_TRACKS]; //linked
	//some useful vars for machine processing
	int datalength;
	_xi_sample s;
	//(...)
	//some useful vars for track processing
	_xi_track tv[_xi_TRACKS];
}_xi_machine;


#pragma pack()

//init procedure
void * _xi_init(char *msd)
{
	int i,size;
	_xi_machine *m=(_xi_machine*)bmx_alloc(sizeof(_xi_machine));

	if (msd!=NULL)
	{
		memcpy(&m->s, msd+4, sizeof(_xi_sample));
		m->s.stream=(unsigned char*)(msd+4+sizeof(_xi_sample) - 8*(sizeof(void*) - 4));

		size=m->s.samplelength;

		if (m->s.compression==1)  // 4 bit
		{
			//bmx_free(m->s.wave); //TODO: crash!

			m->s.wave=(unsigned char*)bmx_alloc(size);

			for (i=0;i<size/2;i++)
			{
				m->s.wave[i*2] = (m->s.stream[i]&0x0F)<<4;
				m->s.wave[i*2+1] = m->s.stream[i]&0xF0;
			}
		}

		else m->s.wave = m->s.stream;

	}
	else m->s.samplelength=0;

	m->tv[0].basefreq=(float)261.7;
	m->tv[0].sn=-1;

	return m;
}

void _xi_startenv(_xi_machine *m)
{
	_xi_track * t;

	if (m->s.volpts==0) return;

	t=&m->tv[0];
	t->da=0;
	t->tick=0;
	t->last_tick=-1;

	//6 - xi's propertiary magnifier
	t->tps=1.0f/(float)BmxSamplesPerTick*6;

	t->a = (float)m->s.volenv[0].y/64.0f;
	t->env_index=0;
}

float _xi_nextenv(_xi_machine *m)
{
	int i;
	float y, k;

	_xi_track *t=&m->tv[0];

	i = t->env_index; if (i==m->s.volpts)
	{
		t->play=FALSE;
		return 0;
	}

	k=m->s.volenv[i].x;

	t->tick += t->tps;

	if ( t->tick >= k && t->last_tick < k )
	{
		k=m->s.volenv[i+1].x;
		y=m->s.volenv[i+1].y/64.0f;
		t->da = (y - t->a) * t->tps / (k - t->tick);
		t->env_index++;
	}
	else t->a += t->da;

	t->last_tick=t->tick;

	return t->a;
}

int _xi_tick(_xi_machine *m, _xi_gpar *gp, _xi_tpar *tp)
{
	int track=0;
	_xi_track *t=&m->tv[0];
	(void)gp;

	if (tp->note!=0)
	{
		t->freq = getfreq(tp[track].note);
		t->sn=0;
		t->dsn = (int)(t->freq/t->basefreq * 256.0);
		_xi_startenv(m);
		t->play=TRUE;
	}
	return 0;
}

BOOL _xi_work(_xi_machine *m, float *psamples, int numsamples, int channels)
{
	int i,track=0;
	_xi_track *t;
	int length,index;
	int a1,a2,a;
	int sndf;

	char *s=(char*)m->s.wave;
	int type=m->s.type;
	int lpstart = m->s.loopstart;
	int lpend = (m->s.loopstart + m->s.looplength);
	(void)channels;

	t=&m->tv[track];
	length=m->s.samplelength;

	if (t->basefreq==0 || t->sn==-1)
		return FALSE;

	for (i=0;i<numsamples;i++)
	{
		index = t->sn >> 8;

		if (index < length && t->play)
		{
			a1=s[index]; a2=s[index+1];
			a = a1*256 + (a2-a1) * (t->sn & 0x000000FF);


			if (type!=0)
				psamples[i] = a * _xi_nextenv(m);
			else
				psamples[i] = (float)a;


			sndf = (t->sn + t->dsn) >> 8;

			//forward loop
			if (type==1)
			{
				if (sndf>=lpend) t->sn=lpstart*256;
			}

			//pingpong loop
			if (type==2)
			{
				if (sndf>=lpend) t->dsn=-t->dsn;
				else
				if (sndf<=lpstart && t->dsn<0)
					t->dsn=-t->dsn;
			}


		}
		else psamples[i]=0;

		t->sn += t->dsn;
	}

	return TRUE;

}

//machine info
BmxMachineHeader _xi_header={
	"_xi", //dllname
	0, //gpsize in bytes
	1, //tpsize in bytes
	1, //channels (1-mono, 2-stereo)
	(LPFINIT)&_xi_init,
	(LPFTICK)&_xi_tick,
	(LPFWORK)&_xi_work
};
#pragma pack()
