//BmxPlay project (c) 2001-2012 Joric^Proxium

//Windows waveout driver
//based on WinUsm waveout code by Nix^TBL

#ifdef _MSC_VER
#pragma comment (lib,"winmm")
#endif

#include <windows.h>
#include <mmsystem.h>

// increment NUMBUFS if player sounds jumpy

#define NUMBUFS 8		//quantity of buffers
#define BUFSIZE 4096		//buffer length in samples (samples per timeslice)

//////////////////////////////////////////////

int BmxWorkBuffer(int samples, short *dest);

LARGE_INTEGER qpcb, qpce;

char bufs[NUMBUFS][BUFSIZE * 4];
#define WAVEOUTLENGTH (BUFSIZE*NUMBUFS)

static volatile int weHaveTerminated = 0;
static volatile int playing = 0;
HANDLE musicevent;
HWAVEOUT audiodev;
int nextbuf;

static void CALLBACK bufferdone(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	if (uMsg == WOM_DONE)
	{
		SetEvent(musicevent);
	}
	return;
}

int PlayThread()
{
	int i;

	WAVEFORMATEX waveformat;
	WAVEHDR waveheader[NUMBUFS];

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	waveformat.nChannels = 2;
	waveformat.wBitsPerSample = 16;
	waveformat.nSamplesPerSec = 44100;
	waveformat.nBlockAlign = (waveformat.nChannels * waveformat.wBitsPerSample) / 8;
	waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;

	waveOutOpen(&audiodev, WAVE_MAPPER, &waveformat, (unsigned int) bufferdone, 0, CALLBACK_FUNCTION);

	musicevent = CreateEvent(NULL, FALSE, FALSE, "BmxPlayer");

	for (i = 0; i < NUMBUFS; i++)
	{
		waveheader[i].lpData = (char *) bufs[i];
		waveheader[i].dwFlags = 0;
		waveheader[i].dwBufferLength = BUFSIZE * 4;
		BmxWorkBuffer(BUFSIZE, (short *) bufs[i]);
		waveOutPrepareHeader(audiodev, &waveheader[i], sizeof(WAVEHDR));
		waveOutWrite(audiodev, &waveheader[i], sizeof(WAVEHDR));
	}

	nextbuf = 0;
	playing = 1;
	while (playing)
	{
		if (WaitForSingleObject(musicevent, 1000) == WAIT_TIMEOUT)
		{

#ifdef _DEBUG
			printf(NULL, "Sound system initialization is timed out.", "", MB_OK);
#endif
			playing = 0;
		}


		for (i = 0; i < NUMBUFS; i++)
			if ((waveheader[i].dwFlags & WHDR_DONE) && nextbuf == i)
			{
				int ticks_in, ticks_out;

				waveOutUnprepareHeader(audiodev, &waveheader[i], sizeof(WAVEHDR));

				QueryPerformanceCounter(&qpcb);

				ticks_out = qpcb.LowPart - qpce.LowPart;

				BmxWorkBuffer(BUFSIZE, (short *) bufs[i]);

				QueryPerformanceCounter(&qpce);
				ticks_in = qpce.LowPart - qpcb.LowPart;
				BmxPerfomance = (float) ticks_in / (float) ticks_out *100.0f;

				waveOutPrepareHeader(audiodev, &waveheader[i], sizeof(WAVEHDR));
				waveOutWrite(audiodev, &waveheader[i], sizeof(WAVEHDR));
				nextbuf++;
				if (nextbuf == NUMBUFS)
					nextbuf = 0;
			}
	}

	waveOutReset(audiodev);

	for (i = 0; i < NUMBUFS; i++)
	{
		waveOutUnprepareHeader(audiodev, &waveheader[i], sizeof(WAVEHDR));
		memset(bufs[i], 0, BUFSIZE * 4);
	}

	waveOutReset(audiodev);
	waveOutClose(audiodev);
	weHaveTerminated = 1;
	return 0;
}

void BmxPlay(void)
{

	DWORD threadid;
	CreateThread((LPSECURITY_ATTRIBUTES) NULL, 0, (LPTHREAD_START_ROUTINE) PlayThread, 0, 0, &threadid);
}

void BmxStop(void)
{
	if (playing)
	{
		playing = 0;
		while (weHaveTerminated == 0);
		weHaveTerminated = 0;
	}
}
