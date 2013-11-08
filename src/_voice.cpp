#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <Windows.h>

#include "MachineInterface.h"
#include "bmxmi.h"
#include "_voice.h"

#pragma optimize ("a", on)

CMachineParameter const paraTune = {
	pt_byte,		// type
	"Tune",
	"Tune (0-80)",		// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	MPF_STATE,		// Flags        
	0x40			//default value
};

CMachineParameter const paraCutoff = {
	pt_byte,		// type
	"Cutoff",
	"Cutoff (0-80)",	// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	MPF_STATE,		// Flags        
	0x40			//default value
};

CMachineParameter const paraResonance = {
	pt_byte,		// type
	"Resonance",
	"Resonance (0-80)",	// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	MPF_STATE,		// Flags        
	0x0			//default value
};


CMachineParameter const paraEnvMod = {
	pt_byte,		// type
	"EnvMod",
	"EnvMod (0-80)",	// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	MPF_STATE,		// Flags        
	0x0			//default value
};

CMachineParameter const paraDecay = {
	pt_byte,		// type
	"Decay",
	"Decay (0-80)",		// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	MPF_STATE,		// Flags        
	0x40			//default value
};

CMachineParameter const paraAccent = {
	pt_byte,		// type
	"Accent",
	"Accent (0-80)",	// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	MPF_STATE,		// Flags        
	0x0			//default value
};

CMachineParameter const paraNote = {
	pt_note,		// type
	"Note",			// short (one or two words) name 
	"Note",			// longer description 
	NOTE_MIN,		// always same for notes
	NOTE_MAX,		// "
	NOTE_NO,		// "
	0,			// flags
	0x30
};

CMachineParameter const paraSlide = {
	pt_byte,		// type
	"Slide",
	"Length of slide in ticks",	// description
	0,			// MinValue     
	0x80,			// MaxValue
	0xFF,			// NoValue
	0,			// Flags        
	0x0			//default value
};

CMachineParameter const paraEndNote = {
	pt_note,		// type
	"Note",			// short (one or two words) name 
	"Slide end note",	// longer description 
	NOTE_MIN,		// always same for notes
	NOTE_MAX,		// "
	NOTE_NO,		// "
	0,			// flags
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

CMachineInfo const MacInfo = {
	MT_GENERATOR,		// type
	MI_VERSION,
	0,			// flags
	1,			// min tracks
	_voice_TRACKS,		// max tracks
	6,			// numGlobalParameters
	3,			// numTrackParameters
	pParameters,
	0,
	NULL,
	"BmxPlay _voice",
	"_voice",		// short name
	"Joric",		// author
	"Open Editor..."
};

class mi;

HINSTANCE g_hInstance;

class mi:public CMachineInterface
{
      public:
	mi();
	virtual ~ mi();

	virtual void Init(CMachineDataInput * const pi);
	virtual void Tick();
	virtual bool Work(float *psamples, int numsamples, int const mode);
	virtual char const *DescribeValue(int const param, int const value);
	virtual void Command(int const i);
	bool OnDoubleClick(void *);
      public:
	long pos;
	float freq;
	int numTracks;
	gvals gval;
	tvals tval[_voice_TRACKS];
	CMachineDataOutput *po;
	_voice_machine *me;
	int current;
	char *data;
	long datasize;
};

_voice_machine *me;

DLL_EXPORTS mi::mi()
{
	GlobalVals = &gval;
	TrackVals = tval;
	AttrVals = NULL;
}

mi::~mi()
{
	free(me);
}

bool mi::OnDoubleClick(void *)
{
	Command(0);
	return true;
}

unsigned int getVTableSize(void *vtable)
{
	void **funcptr = (void **) vtable;

	unsigned int count = 0;

	while (IsBadCodePtr((FARPROC) * funcptr) == FALSE)
	{
		++count;
		++funcptr;
	}

	return count;
}

void mi::Init(CMachineDataInput * const pi)
{
	if (getVTableSize(*(void ***) pCB) >= 50)	//check for buzz >= 1.2
		pCB->SetEventHandler(pCB->GetThisMachine(), DoubleClickMachine,
		(bool(CMachineInterface::*)(void *))&mi::OnDoubleClick, NULL);

	BmxSamplesPerSecond = pMasterInfo->SamplesPerSec;
	me = (_voice_machine *) _voice_init(NULL);
}

void mi::Tick()
{
	BmxSamplesPerTick = pMasterInfo->SamplesPerTick;
	_voice_tick(me, (_voice_gpar *) & gval, (_voice_tpar *) & tval);
}


bool mi::Work(float *psamples, int numsamples, int const)
{
	return _voice_work(me, psamples, numsamples, 1);
}

char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];
	sprintf(txt, "%.02X", value);
	return txt;
}
/*
BOOL APIENTRY VoiceDlgProc(HWND hDlg, WORD msg, WORD wParam, LONG lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{

			int i;
			int c;
			char buf[256];

			c = sizeof(_presets) / sizeof(_presets[0]);

			for (i = 0; i < c; i++)
			{
				_voice_preset *p = &_presets[i];
				sprintf(buf, "%c", p->ch);
				SendDlgItemMessage(hDlg, IDC_LIST2, LB_ADDSTRING, 0, (DWORD) (LPSTR) buf);
			}

			c = sizeof(_phrases) / sizeof(_phrases[0]);

			for (i = 0; i < c; i++)
			{
				sprintf(buf, "%02d. %s", i, _phrases[i]);
				SendDlgItemMessage(hDlg, IDC_LIST1, LB_ADDSTRING, 0, (DWORD) (LPSTR) buf);
			}

			SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETRANGE, TRUE, MAKELONG(0, 120));
			SendDlgItemMessage(hDlg, IDC_SLIDER2, TBM_SETRANGE, TRUE, MAKELONG(0, 120));
			SendDlgItemMessage(hDlg, IDC_SLIDER3, TBM_SETRANGE, TRUE, MAKELONG(0, 300));
			SendDlgItemMessage(hDlg, IDC_SLIDER4, TBM_SETRANGE, TRUE, MAKELONG(0, 300));
			SendDlgItemMessage(hDlg, IDC_SLIDER5, TBM_SETRANGE, TRUE, MAKELONG(0, 10000));
			SendDlgItemMessage(hDlg, IDC_SLIDER6, TBM_SETRANGE, TRUE, MAKELONG(0, 10000));
			SendDlgItemMessage(hDlg, IDC_SLIDER7, TBM_SETRANGE, TRUE, MAKELONG(0, 255));
			SendDlgItemMessage(hDlg, IDC_SLIDER8, TBM_SETRANGE, TRUE, MAKELONG(0, 10000));

		}

//			StatusWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE, "Now Playing", hDlg, 999);
//			SendMessage (StatusWnd, SB_SETPARTS, 3, (LPARAM)"test");

			break;

		case WM_VSCROLL:
		{
			int i;
			int nItem;
			char s[256];
			HWND hwndList = GetDlgItem(hDlg, IDC_LIST2);
			nItem = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			SendMessage(hwndList, LB_GETTEXT, nItem, (LONG) s);
			_voice_preset *p = get_p(s[0]);
			p->a1 = (int) SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_GETPOS, 0, 0);
			p->a2 = (int) SendDlgItemMessage(hDlg, IDC_SLIDER2, TBM_GETPOS, 0, 0);
			p->b1 = (int) SendDlgItemMessage(hDlg, IDC_SLIDER3, TBM_GETPOS, 0, 0);
			p->b2 = (int) SendDlgItemMessage(hDlg, IDC_SLIDER4, TBM_GETPOS, 0, 0);
			p->f1 = (int) SendDlgItemMessage(hDlg, IDC_SLIDER5, TBM_GETPOS, 0, 0);
			p->f2 = (int) SendDlgItemMessage(hDlg, IDC_SLIDER6, TBM_GETPOS, 0, 0);
			p->asp = (int) SendDlgItemMessage(hDlg, IDC_SLIDER7, TBM_GETPOS, 0, 0);
			p->len = (int) SendDlgItemMessage(hDlg, IDC_SLIDER8, TBM_GETPOS, 0, 0);
		}

			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{

			case IDC_LIST1:
				{
					char s[256];
					HWND hwndList = GetDlgItem(hDlg, IDC_LIST1);
					int nItem = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
					SendMessage(hwndList, LB_GETTEXT, nItem, (LONG) s);
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), s+4);
				}			
				break;

				case IDC_LIST2:
				{
					int i;
					int nItem;
					char s[256];
					HWND hwndList = GetDlgItem(hDlg, IDC_LIST2);
					nItem = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
					SendMessage(hwndList, LB_GETTEXT, nItem, (LONG) s);
					SetWindowText(GetDlgItem(hDlg, IDC_EDIT2), s);

					_voice_preset *p = get_p(s[0]);

					SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_SETPOS, TRUE, (LONG) p->a1);
					SendDlgItemMessage(hDlg, IDC_SLIDER2, TBM_SETPOS, TRUE, (LONG) p->a2);
					SendDlgItemMessage(hDlg, IDC_SLIDER3, TBM_SETPOS, TRUE, (LONG) p->b1);
					SendDlgItemMessage(hDlg, IDC_SLIDER4, TBM_SETPOS, TRUE, (LONG) p->b2);
					SendDlgItemMessage(hDlg, IDC_SLIDER5, TBM_SETPOS, TRUE, (LONG) p->f1);
					SendDlgItemMessage(hDlg, IDC_SLIDER6, TBM_SETPOS, TRUE, (LONG) p->f2);
					SendDlgItemMessage(hDlg, IDC_SLIDER7, TBM_SETPOS, TRUE, (LONG) p->asp);
					SendDlgItemMessage(hDlg, IDC_SLIDER8, TBM_SETPOS, TRUE, (LONG) p->len);
				}
					break;

				case IDCANCEL:
					EndDialog(hDlg, TRUE);
					return (TRUE );
					break;
					
				case IDOK:
					EndDialog(hDlg, TRUE);
					return (TRUE);
					break;
					
			}
			break;

	}

	return false;
}
*/

void mi::Command(int const i)
{
	::me = me;
	//::lpmi=this;

	HWND hOld = FindWindow(NULL, "_voice");
	if (hOld)
	{
		SetForegroundWindow(hOld);
	}
	else
	{
		//DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_voice), NULL, (DLGPROC) VoiceDlgProc);
		MessageBox(NULL, "Not implemented", "_voice", MB_OK);
	}
}



BOOL WINAPI DllMain(HANDLE hModule, DWORD dwFunction, LPVOID lpNot)
{
	g_hInstance = (HINSTANCE) hModule;

	return TRUE;
}
