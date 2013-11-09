#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <Windows.h>

#include "MachineInterface.h"
#include "bmxmi.h"
#include "_xi.h"

//#include <commctrl.h>
//#pragma comment(lib,"comdlg32")

#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")

#pragma optimize ("a", on)

#pragma warning (disable: 4005 4700) //stc32 redefinition, local variable, blablabla


char executable[256];
HWND hSMWnd;


HINSTANCE g_hInstance=NULL;
HWND g_hWnd=NULL;
	HWND hWndList=NULL;
	static const char *m_AppName	= "_xi";
	static const char *m_AppTitle	= "_xi Title";	


#pragma optimize ("a", on)

#define MAX_TRACKS 8


CMachineParameter const paraNote = 
{ 
	pt_note,                    // type
	"Note",                     // short (one or two words) name 
	"Note",                     // longer description 
	NOTE_MIN,                   // always same for notes
	NOTE_MAX,                   // "
	NOTE_NO,                    // "
	0,                  // flags
	NOTE_NO
};


CMachineParameter const *pParameters[] = { 
	&paraNote,
};


CMachineAttribute const attrCompressed = 
{
	"Compress (4-bit)",
	0,
	1,
	0
};

CMachineAttribute const *pAttributes[] = 
{
	&attrCompressed
};

#pragma pack(1)

class tvals
{
public:
	byte note;
};

class avals
{
public:
	int compressed;
};

typedef unsigned char   UBYTE;          // has to be 1 byte unsigned
typedef unsigned short  UWORD;          // has to be 2 bytes unsigned
typedef long            LONG;           // has to be 4 bytes signed
typedef unsigned long   ULONG;          // has to be 4 bytes unsigned


typedef struct XMEXTINSTHEADER{  // structure for saving/loading instruments
	char isign[21];//"Extended Instrument: ";
	char name[22];
	char term; //always 0x1A
	char msign[20];//"FastTracker v2.00   ";
	UWORD size;                             // (dword) Instrument size
}XMEXTINSTHEADER;//65 bytes

typedef struct XMPATCHHEADER{
	UBYTE what[96];         // (byte) Sample number for all notes
	UBYTE volenv[48];       // (byte) Points for volume envelope
	UBYTE panenv[48];       // (byte) Points for panning envelope
	UBYTE volpts;           // (byte) Number of volume points
	UBYTE panpts;           // (byte) Number of panning points
	UBYTE volsus;           // (byte) Volume sustain point
	UBYTE volbeg;           // (byte) Volume loop start point
	UBYTE volend;           // (byte) Volume loop end point
	UBYTE pansus;           // (byte) Panning sustain point
	UBYTE panbeg;           // (byte) Panning loop start point
	UBYTE panend;           // (byte) Panning loop end point
	UBYTE volflg;           // (byte) Volume type: bit 0: On; 1: Sustain; 2: Loop
	UBYTE panflg;           // (byte) Panning type: bit 0: On; 1: Sustain; 2: Loop
	UBYTE vibflg;           // (byte) Vibrato type
	UBYTE vibsweep;         // (byte) Vibrato sweep
	UBYTE vibdepth;         // (byte) Vibrato depth
	UBYTE vibrate;          // (byte) Vibrato rate
	UWORD volfade;          // (word) Volume fadeout
	UWORD reserved[11];     // (word) Reserved
} XMPATCHHEADER;

typedef struct XMWAVHEADER{
	ULONG length;           // (dword) Sample length
	ULONG loopstart;        // (dword) Sample loop start
	ULONG looplength;       // (dword) Sample loop length
	UBYTE volume;           // (byte) Volume
	BYTE finetune;          // (byte) Finetune (signed byte -128..+127)
	UBYTE type;                     // (byte) Type: Bit 0-1: 0 = No loop, 1 = Forward loop,
//                                        2 = Ping-pong loop;
//                                        4: 16-bit sampledata
	UBYTE panning;          // (byte) Panning (0-255)
	BYTE  relnote;          // (byte) Relative note number (signed byte)
	UBYTE reserved;         // (byte) Reserved
	char  samplename[22];   // (char) Sample name
} XMWAVHEADER;


#pragma pack()

CMachineInfo const MacInfo = 
{
	MT_GENERATOR,							// type
	MI_VERSION,
	0,										// flags
	1,										// min tracks
	MAX_TRACKS,								// max tracks
	0,										// numGlobalParameters
	1,										// numTrackParameters
	pParameters,
	1, 
	pAttributes,
#ifdef _DEBUG
	"bmxplay _xi (Debug build)",		// name
#else
	"bmxplay _xi",
#endif
	"_xi", // short name
	"Joric", // author
	"Import xi..."
};

_xi_machine *me;

class mi;

mi *lpmi;

class mi : public CMachineInterface
{
public:
	mi();
	virtual ~mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual void Command(int const i);	
	virtual void Save(CMachineDataOutput * const po);
	void OnLoad();
	void OnImport(char *filename);
	virtual void AttributesChanged();

public:
	long pos;
	float freq;
	int numTracks;
	avals aval;
	tvals tval[MAX_TRACKS];
	CMachineDataOutput *po;
	_xi_machine *me;	
	char *data;
	unsigned char *wave;
	unsigned char *stream;
	int current;

	XMEXTINSTHEADER eih;
	XMPATCHHEADER pth;
	XMWAVHEADER wh;

};

DLL_EXPORTS

mi::mi()
{
	GlobalVals = NULL;
	TrackVals = tval;
	AttrVals = (int *)&aval;
}

mi::~mi()
{
	free(me);
}


void mi::Init(CMachineDataInput * const pi)
{
	int datasize;
	
	if (pi==NULL)
	{
		me=(_xi_machine*)_xi_init(NULL);

	}
	else
	{
		//a really crazy stuff - buzz can't tell us how big the data is
		//but if size not fixed - we have to know how much memory we'll allocate for it
		//so i've attached datasize (4 bytes integer) before actual data
		pi->Read(datasize);
		data=(char*)malloc(datasize+4);
		pi->Read(data+4,datasize); 
		me=(_xi_machine*)_xi_init(data);

		aval.compressed = me->s.compression;
	}

}

void mi::Tick()
{
	BmxSamplesPerTick=pMasterInfo->SamplesPerTick;
	_xi_tick(me,NULL,(_xi_tpar*)&tval);
}


bool mi::Work(float *psamples, int numsamples, int const)
{	
	return _xi_work(me,psamples,numsamples,1)!=0;
}


void mi::OnLoad()
{
	TCHAR szDir[MAX_PATH] = TEXT("");
	TCHAR szFile[MAX_PATH] = TEXT("");
	strcpy(szFile,"*.xi");
	GetCurrentDirectory(MAX_PATH,szDir);
	OPENFILENAME   ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize         = sizeof(ofn);
	ofn.hwndOwner           = NULL;
	ofn.hInstance           = g_hInstance;
	ofn.lpstrFilter         = "*.xi";
	ofn.lpstrCustomFilter   = NULL;
	ofn.lpstrFile           = szFile;
	ofn.nMaxFile            = MAX_PATH;
	ofn.lpstrInitialDir     = szDir;
	ofn.lCustData           = 0;
	if (GetOpenFileName(&ofn))
		OnImport(ofn.lpstrFile);
}

void mi::OnImport(char *filename)
{
	int i;
	int size;
	unsigned char l,h;

	FILE *fp;
	if (fp=fopen(filename,"rb"))
	{
		fread(&eih,sizeof(XMEXTINSTHEADER),1,fp);
		fread(&pth,sizeof(XMPATCHHEADER),1,fp);
		short a; fread(&a,sizeof(short),1,fp);
		fread(&wh,sizeof(XMWAVHEADER),1,fp);

		size = wh.length;

		//free (wave); //TODO: crash!

		wave = (unsigned char*)malloc(size);

		fread(wave,size,1,fp);

		char old,n;
		old=0;
		for (i=0; i<size;i++)
		{
			n=wave[i]+old;
			wave[i]=n;
			old=n;
		}

		me->s.samplelength=size;
		me->s.volpts=pth.volpts;

		memcpy(me->s.volenv, pth.volenv, 48);

		me->s.volflg    = pth.volflg;
		me->s.type      = wh.type;
		me->s.compression = aval.compressed;
		me->s.relnote   = wh.relnote;
		me->s.finetune  = wh.finetune;
		me->s.loopstart = wh.loopstart;
		me->s.looplength = wh.looplength;
		me->s.wave=(unsigned char*)wave;

		if (me->s.compression==1)
		{
			//bmx_free(stream); //TODO: crash

			stream=(unsigned char*)malloc(size/2);

			for (i=0;i<size/2;i++)
			{
				l=wave[i*2];
				h=wave[i*2+1];

				stream[i] = ((h&0xF0)) | ((l&0xF0)>>4);
			}

			memset(wave,0,size);

			for (i=0;i<size/2;i++)
			{
				wave[i*2] = (stream[i]&0x0F)<<4;
				wave[i*2+1] = stream[i]&0xF0;
			}

			me->s.stream=stream;

		}
		else
		{
			me->s.stream=me->s.wave;
		}
		fclose(fp);
	}
}

void mi::Command(int const i)
{
	::me=me;
	::lpmi=this;

	OnLoad();
}

void mi::AttributesChanged()
{
	me->s.compression = aval.compressed;
}

void mi::Save(CMachineDataOutput * const po)
{

	int size=me->s.samplelength;

	int datasize;

	if (size!=0)
	{
		if (me->s.compression==1) size=size/2;

		datasize = sizeof(_xi_sample) + size;
		po->Write(datasize);
		po->Write(&me->s,sizeof(_xi_sample));
		po->Write(me->s.stream, size);
	}
	
}

BOOL WINAPI DllMain (HANDLE hModule, DWORD dwFunction, LPVOID lpNot)
{
	g_hInstance=(HINSTANCE)hModule;		

	return TRUE;
}

