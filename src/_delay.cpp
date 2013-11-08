#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <Windows.h>

#include "MachineInterface.h"
#include "bmxmi.h"
#include "_delay.h"

CMachineParameter const paraLength = 
{ 
	pt_byte,										// type
	"Length",
	"Length (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x2D	//default value
};

CMachineParameter const paraWetout = 
{ 
	pt_byte,										// type
	"Wet out",
	"Wet out (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0x67 //default value
};

CMachineParameter const paraDryout = 
{ 
	pt_byte,										// type
	"Dry out",
	"Dry out (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0x80 //default value
};


CMachineParameter const paraFeedback = 
{ 
	pt_byte,										// type
	"Feedback",
	"Feedback (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0x5D //default value
};

CMachineParameter const *pParameters[] = 
{ 
	&paraLength,
	&paraFeedback,
	&paraDryout,
	&paraWetout
};

#pragma pack(1)

class gvals
{
public:
	byte length;
	byte feedback;
	byte wetout;	
	byte dryout;
};

#pragma pack()


CMachineInfo const MacInfo = 
{
	MT_EFFECT,								// type
	MI_VERSION,
	MIF_MONO_TO_STEREO,						// flags
	0,										// min tracks
	0,										// max tracks
	4,										// numGlobalParameters
	0,										// numTrackParameters
	pParameters,
	0,
	NULL,
#ifdef _DEBUG
	"bmxplay _delay",			// name
#else
	"bmxplay _delay",
#endif
	"_delay",						// short name
	"Joric",						// author
	NULL
};
 
class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode);
	virtual char const *DescribeValue(int const param, int const value);
private:
	gvals gval;
	_delay_machine *me;
};

DLL_EXPORTS

mi::mi()
{
	GlobalVals = &gval;
	TrackVals = NULL;
	AttrVals = NULL;
}

mi::~mi()
{
	//bmx_free(me->buf);
	bmx_free(me);
}

int current;

void mi::Init(CMachineDataInput * const pi)
{	

	me=(_delay_machine*)_delay_init(NULL);
}

void mi::Tick()
{
	_delay_tick(me,(_delay_gpar*)&gval,NULL);
}


bool mi::WorkMonoToStereo(float *pin, float *pout, int numsamples, int mode)
{	

	BmxSamplesPerTick=pMasterInfo->SamplesPerTick;
	
	/*
	we gonna do mono to stereo conversion
	we have function with independent input and output,
	but the bmxplay uses a single stream.
	the only way is to fill left channel by input, 
	and pass it to bmxplay filter
	this shit eats about 0.3% of CPU on my sextium
	*/

	//stretching mono to left channel of stereo stream
	for (int i=0; i<numsamples; i++) pout[i*2]=pin[i];

	//the usual jeskola's piece of shit
	float *paux = pCB->GetAuxBuffer();
	if (mode & WM_READ) memcpy(paux, pout, numsamples*8); else memset (pout,0,numsamples*8);

	//processing
	return _delay_work(me,pout,numsamples,2);
}


char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];
	sprintf(txt, "%.02X", value);
	return txt;
}
