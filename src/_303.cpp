#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <Windows.h>

#include "MachineInterface.h"
#include "bmxmi.h"
#include "_303.h"

#pragma optimize ("a", on)

CMachineParameter const paraTune = 
{ 
	pt_byte,										// type
	"Tune",
	"Tune (0-80)",									// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x40	//default value
};

CMachineParameter const paraCutoff = 
{ 
	pt_byte,										// type
	"Cutoff",
	"Cutoff (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x40	//default value
};

CMachineParameter const paraResonance = 
{ 
	pt_byte,										// type
	"Resonance",
	"Resonance (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x0	//default value
};


CMachineParameter const paraEnvMod = 
{ 
	pt_byte,										// type
	"EnvMod",
	"EnvMod (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x0	//default value
};

CMachineParameter const paraDecay = 
{ 
	pt_byte,										// type
	"Decay",
	"Decay (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x40	//default value
};

CMachineParameter const paraAccent = 
{ 
	pt_byte,										// type
	"Accent",
	"Accent (0-80)",								// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags	
	0x0	//default value
};

CMachineParameter const paraNote = 
{ 
	pt_note,					// type
	"Note",						// short (one or two words) name 
	"Note",						// longer description 
	NOTE_MIN,					// always same for notes
	NOTE_MAX,					// "
	NOTE_NO,					// "
	0,							// flags
	0x30
};

CMachineParameter const paraSlide = 
{ 
	pt_byte,										// type
	"Slide",
	"Length of slide in ticks",						// description
	0,												// MinValue	
	0x80,											// MaxValue
	0xFF,											// NoValue
	0,										// Flags	
	0x0	//default value
};

CMachineParameter const paraEndNote = 
{ 
	pt_note,					// type
	"Note",						// short (one or two words) name 
	"Slide end note",			// longer description 
	NOTE_MIN,					// always same for notes
	NOTE_MAX,					// "
	NOTE_NO,					// "
	0,							// flags
	0x30
};

CMachineParameter const *pParameters[] = { 
	&paraTune,
	&paraCutoff,
	&paraResonance,
	&paraEnvMod,
	&paraDecay,
	&paraAccent,
	&paraNote,
	&paraSlide,
	&paraEndNote
};

#pragma pack(1)


class gvals
{
public:
	byte tune;
	byte cutoff;
	byte resonance;
	byte envmod;
	byte decay;
	byte accent;
};

class tvals
{
public:
	byte note;
	byte slide;
	byte endnote;
};

#pragma pack()

CMachineInfo const MacInfo = 
{
	MT_GENERATOR,							// type
	MI_VERSION,
	0,										// flags
	1,										// min tracks
	_303_TRACKS,							// max tracks
	6,										// numGlobalParameters
	3,										// numTrackParameters
	pParameters,
	0, 
	NULL,
	"BmxPlay _303",
	"_303_", // short name
	"Joric", // author
	NULL
};

class mi;

class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual char const *DescribeValue(int const param, int const value);
public:
	long pos;
	float freq;
	int numTracks;
	gvals gval;
	tvals tval[_303_TRACKS];
	CMachineDataOutput *po;
	_303_machine *me;
	int current;
	char *data;
	long datasize;
};

DLL_EXPORTS

mi::mi()
{
	GlobalVals = &gval;
	TrackVals = tval;
	AttrVals = NULL;
}

mi::~mi()
{
	free(me);
}

void mi::Init(CMachineDataInput * const pi)
{	
	BmxSamplesPerSecond=pMasterInfo->SamplesPerSec;
	me=(_303_machine*)_303_init(NULL);
}

void mi::Tick()
{
	BmxSamplesPerTick=pMasterInfo->SamplesPerTick;
	_303_tick(me,(_303_gpar*)&gval,(_303_tpar*)&tval);
}


bool mi::Work(float *psamples, int numsamples, int const)
{	
	return _303_work(me,psamples,numsamples,1)!=0;
}

char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];
	sprintf(txt, "%.02X", value);
	return txt;
}
