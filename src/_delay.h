//_delay machine for bmxplay

#define _delay_TRACKS 0

//1 sec buffer
#define _delay_BUFFER 44100

#pragma pack(1)

//global parameters
typedef struct{
	byte length;
	byte feedback;
	byte dryout;
	byte wetout;
}_delay_gpar;

//track parameters
typedef struct{
	char dummy;
}_delay_tpar;


//track info
typedef struct{
	//some useful vars for track processing
	//(...)
	char dummy;
}_delay_track;

//machine info
typedef struct{
	int tracks; //number of tracks
	_delay_gpar gpar;
	float dbuf[_delay_BUFFER*2];
	//some useful vars for machine processing
	float *buf, *lbuf, *rbuf, length, wetout, feedback, dryout;
	char pan;
	int writepos;
	//(...)
	//some useful vars for track processing
}_delay_machine;


#pragma pack()


//init procedure
void *_delay_init(char *msd)
{
	_delay_machine *m=(_delay_machine*)bmx_alloc(sizeof(_delay_machine));
	m->buf=(float*)m->dbuf;
	memset(m->buf,0,_delay_BUFFER*2*4);
	m->writepos=0;
	m->pan=0;
	return m;
}

int _delay_tick(_delay_machine *m, _delay_gpar *gp, _delay_tpar *tp)
{
	if (gp->length!=0xFF) m->length = gp->length/128.0f;
	if (gp->wetout!=0xFF) m->wetout = gp->wetout/128.0f;
	if (gp->dryout!=0xFF) m->dryout = gp->dryout/128.0f;
	if (gp->feedback!=0xFF) m->feedback = gp->feedback/128.0f;

	return 0;
}


BOOL _delay_work(_delay_machine *m, float *psamples, int numsamples, int channels)
{
	int i;
	int iw;
	float *lbuf, *rbuf;
	int delta = (int)(m->length * _delay_BUFFER);
	float wetout=m->wetout;
	float dryout=m->dryout;
	float feedback=m->feedback;
	float *p=psamples;
	char pan = m->pan;
	float *buf=m->buf;

	if (pan!=0) {
		lbuf=m->buf; rbuf=m->buf+_delay_BUFFER;
	} else {
		rbuf=m->buf; lbuf=m->buf+_delay_BUFFER;
	}

	iw=m->writepos;
	lbuf+=iw;
	rbuf+=iw;
	buf+=iw;

	//erm... the fastest possible :-))

	for (i=0;i<numsamples;i++)
	{
		float in = *p;

		*p++ = *rbuf * wetout + in * dryout;
		*rbuf = /*  in +  */ feedback * *rbuf;

		*p++ = *lbuf * wetout + in * dryout;
		*lbuf= in + feedback * *lbuf;

		rbuf++;
		lbuf++;

		iw++;

		if (iw>=delta)
		{
			iw=0; pan=1-pan;
			if (pan!=0) { lbuf=m->buf; rbuf=m->buf+_delay_BUFFER;}
					else { rbuf=m->buf; lbuf=m->buf+_delay_BUFFER;}
		}
	}


	m->writepos=iw;
	m->pan=pan;

	return TRUE;
}

#pragma pack(1)
//machine info
BmxMachineHeader _delay_header={
  "_delay", //dllname
  sizeof(_delay_gpar), //gpsize in bytes
  0, //tpsize in bytes
  2,  //channels (1-mono, 2-stereo)
  (LPFINIT)&_delay_init,
  (LPFTICK)&_delay_tick,
  (LPFWORK)&_delay_work,
};
#pragma pack()
