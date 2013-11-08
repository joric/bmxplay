#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <Windows.h>

#include "MachineInterface.h"
#include "bmxmi.h"
#include "_filter.h"

CMachineParameter const paraP1 = 
{ 
	pt_byte,										// type
	"Cutoff",
	"Cutoff (0-80)",							    // description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x80	//default value
};

CMachineParameter const paraP2 = 
{ 
	pt_byte,										// type
	"Resonance",
	"Resonance (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0 //default value
};

CMachineParameter const paraP3 = 
{ 
	pt_byte,										// type
	"Distortion",
	"Distortion (0-80)",							// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0 //default value
};

CMachineParameter const *pParameters[] = 
{ 
	&paraP1,
	&paraP2,
	&paraP3
};


#pragma pack(1)
class gvals
{
public:
	byte param1;
	byte param2;
	byte param3;
};
#pragma pack()

#pragma optimize ("a", on)

CMachineInfo const MacInfo = 
{
	MT_EFFECT,								// type
	MI_VERSION,
	0,										// flags
	0,										// min tracks
	0,										// max tracks
	3,										// numGlobalParameters
	0,										// numTrackParameters
	pParameters,
	0,
	NULL,
#ifdef _DEBUG
	"bmxplay _filter",			// name
#else
	"bmxplay _filter",
#endif
	"_filter",						// short name
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
	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual char const *DescribeValue(int const param, int const value);
private:
	gvals gval;
	_filter_machine *me;
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
	free (me);
}

int current;

void mi::Init(CMachineDataInput * const pi)
{	
	me=(_filter_machine*)_filter_init(NULL);
}

void mi::Tick()
{
	_filter_tick(me,(_filter_gpar*)&gval,NULL);
}


bool mi::Work(float *psamples, int numsamples, int mode)
{	
	if (mode == WM_WRITE || mode == WM_NOIO || mode==WM_READ ) return false;

	return _filter_work(me,psamples,numsamples,1);
}

char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];
	sprintf(txt, "%.02X", value);
	return txt;
}
