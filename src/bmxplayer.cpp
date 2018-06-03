//bmxplayer.cpp (c) 2008 Joric^Proxium

#ifdef _MSC_VER
#pragma comment(lib,"comdlg32")
#pragma comment(lib,"winmm")
#pragma comment(lib,"user32")
#pragma comment(lib,"gdi32")
#endif

#define CLAMP(t, dt, min, max) ((t + dt > max) ? t = max : (t + dt < min) ? t = min : t += dt)
#define WM_MOUSEWHEEL 0x020A
#define	BMP_WIDTH 512
#define	BMP_HEIGHT 256
#define	BMP_SIZE (BMP_WIDTH * BMP_HEIGHT)
#define IDC_LOAD 1024
#define IDC_PAUSE 1025

#define _CRT_SECURE_NO_DEPRECATE

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <io.h>
#include <direct.h>		//getcwd
#include "bmxplay.h"
#include <mmsystem.h>

#include <shobjidl.h>
ITaskbarList3* _taskbar;

//#define USE_ACRYLIC

#ifdef USE_ACRYLIC
struct ACCENTPOLICY
{
	int nAccentState;
	int nFlags;
	int nColor;
	int nAnimationId;
};

struct WINCOMATTRPDATA {
	int nAttribute;
	PVOID pData;
	ULONG ulDataSize;
};

enum WINCOMPATTR {
	WCA_ACCENT_POLICY = 19
};

enum ACCENTTYPES {
	ACCENT_DISABLE = 0,
	ACCENT_ENABLE_GRADIENT = 1,
	ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
	ACCENT_ENABLE_BLURBEHIND = 3,
	ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
	ACCENT_INVALID_STATE = 5
};
#endif

float BmxGetVolume()
{
	MMRESULT result;
	HMIXER hMixer;
	result = mixerOpen(&hMixer, MIXER_OBJECTF_MIXER, 0, 0, 0);

	MIXERLINE ml;
	memset(&ml, 0, sizeof(ml));
	ml.cbStruct = sizeof(ml);
	ml.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	result = mixerGetLineInfo((HMIXEROBJ) hMixer, &ml, MIXER_GETLINEINFOF_COMPONENTTYPE);

	MIXERLINECONTROLS mlc;
	memset(&mlc,0,sizeof(mlc));
	MIXERCONTROL mc;
	memset(&mc,0,sizeof(mc));
	mlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mlc.dwLineID = ml.dwLineID;
	mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mlc.cControls = 1;
	mlc.pamxctrl = &mc;
	mlc.cbmxctrl = sizeof(MIXERCONTROL);
	result = mixerGetLineControls((HMIXEROBJ) hMixer, &mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

	MIXERCONTROLDETAILS mcd;
	memset(&mcd, 0, sizeof(mcd));
	MIXERCONTROLDETAILS_UNSIGNED mcdu;
	memset(&mcdu, 0, sizeof(mcdu));

	mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mcd.hwndOwner = 0;
	mcd.dwControlID = mc.dwControlID;
	mcd.paDetails = &mcdu;
	mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	mcd.cChannels = 1;

	result = mixerGetControlDetails((HMIXEROBJ) hMixer, &mcd, MIXER_SETCONTROLDETAILSF_VALUE);

	return mcdu.dwValue / 65535.0f;
}

void BmxSetVolume(float volume)
{
	MMRESULT result;
	HMIXER hMixer;
	result = mixerOpen(&hMixer, MIXER_OBJECTF_MIXER, 0, 0, 0);

	MIXERLINE ml;
	memset(&ml, 0, sizeof(ml));
	ml.cbStruct = sizeof(MIXERLINE);
	ml.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	result = mixerGetLineInfo((HMIXEROBJ) hMixer, &ml, MIXER_GETLINEINFOF_COMPONENTTYPE);

	MIXERLINECONTROLS mlc;
	memset(&mlc,0,sizeof(mlc));
	MIXERCONTROL mc;
	memset(&mc,0,sizeof(mc));
	mlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mlc.dwLineID = ml.dwLineID;
	mlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mlc.cControls = 1;
	mlc.pamxctrl = &mc;
	mlc.cbmxctrl = sizeof(MIXERCONTROL);
	result = mixerGetLineControls((HMIXEROBJ) hMixer, &mlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

	MIXERCONTROLDETAILS mcd;
	memset(&mcd, 0, sizeof(mcd));
	MIXERCONTROLDETAILS_UNSIGNED mcdu;
	memset(&mcdu, 0, sizeof(mcdu));

	mcdu.dwValue = (DWORD) (volume * 65535.0f);

	mcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mcd.hwndOwner = 0;
	mcd.dwControlID = mc.dwControlID;
	mcd.paDetails = &mcdu;
	mcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	mcd.cChannels = 1;

	result = mixerSetControlDetails((HMIXEROBJ) hMixer, &mcd, MIXER_SETCONTROLDETAILSF_VALUE);
}

int BmxGetPos()
{
	MMTIME mmt;
	mmt.wType = TIME_SAMPLES;
	if (!audiodev)
		return 0;
	waveOutGetPosition(audiodev, &mmt, sizeof(mmt));
	return (mmt.u.sample) % (BUFSIZE * NUMBUFS);
}

//here I use fix_fft.tar.gz by Tom Roberts, Malcolm Slaney and Dimitrios P. Bouras
//taken from http://www.jjj.de/fft/fftpage.html

RECT m_RectDlg;
POINT m_pt;
int m_mx, m_mx0, m_my, m_my0;
bool m_pressed = false;
bool m_captured = false;
int LOG2_N_WAVE = 0;
int N_WAVE = 0;
short *Sinewave = NULL;
bool m_pause = false;

/* init_fft by Joric^Proxium */
int init_fft(short m)
{
	LOG2_N_WAVE = m;
	N_WAVE = 1 << LOG2_N_WAVE;

	int size = N_WAVE - N_WAVE / 4;

	if (Sinewave != NULL)
		free(Sinewave);

	Sinewave = (short *) malloc(size * sizeof(short));

	for (int i = 0; i < size; i++)
		Sinewave[i] = (short) (sin(M_PI * i / size * 1.50f) * 32767.0f);

	return 0;
}

/*
  FIX_MPY() - fixed-point multiplication & scaling.
  Substitute inline assembly for hardware-specific
  optimization suited to a particluar DSP processor.
  Scaling ensures that result remains 16-bit.
*/
inline short FIX_MPY(short a, short b)
{
	/* shift right one less bit (i.e. 15-1) */
	int c = ((int) a * (int) b) >> 14;
	/* last bit shifted out = rounding-bit */
	b = c & 0x01;
	/* last shift + rounding bit */
	a = (c >> 1) + b;
	return a;
}

/*
  fix_fft() - perform forward/inverse fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m; set inverse to
  0 for forward transform (FFT), or 1 for iFFT.
*/
int fix_fft(short fr[], short fi[], short m, short inverse)
{
	int mr, nn, i, j, l, k, istep, n, scale, shift;
	short qr, qi, tr, ti, wr, wi;

	if (LOG2_N_WAVE < m)
		init_fft(m);

	n = 1 << m;

	/* max FFT size = N_WAVE */
	if (n > N_WAVE)
		return -1;

	mr = 0;
	nn = n - 1;
	scale = 0;

	/* decimation in time - re-order data */
	for (m = 1; m <= nn; ++m)
	{
		l = n;
		do
		{
			l >>= 1;
		}
		while (mr + l > nn);
		mr = (mr & (l - 1)) + l;

		if (mr <= m)
			continue;
		tr = fr[m];
		fr[m] = fr[mr];
		fr[mr] = tr;
		ti = fi[m];
		fi[m] = fi[mr];
		fi[mr] = ti;
	}

	l = 1;
	k = LOG2_N_WAVE - 1;
	while (l < n)
	{
		if (inverse)
		{
			/* variable scaling, depending upon data */
			shift = 0;
			for (i = 0; i < n; ++i)
			{
				j = fr[i];
				if (j < 0)
					j = -j;
				m = fi[i];
				if (m < 0)
					m = -m;
				if (j > 16383 || m > 16383)
				{
					shift = 1;
					break;
				}
			}
			if (shift)
				++scale;
		}
		else
		{
			/*
			   fixed scaling, for proper normalization --
			   there will be log2(n) passes, so this results
			   in an overall factor of 1/n, distributed to
			   maximize arithmetic accuracy.
			 */
			shift = 1;
		}
		/*
		   it may not be obvious, but the shift will be
		   performed on each data point exactly once,
		   during this pass.
		 */
		istep = l << 1;
		for (m = 0; m < l; ++m)
		{
			j = m << k;
			/* 0 <= j < N_WAVE/2 */
			wr = Sinewave[j + N_WAVE / 4];
			wi = -Sinewave[j];
			if (inverse)
				wi = -wi;
			if (shift)
			{
				wr >>= 1;
				wi >>= 1;
			}
			for (i = m; i < n; i += istep)
			{
				j = i + l;
				tr = FIX_MPY(wr, fr[j]) - FIX_MPY(wi, fi[j]);
				ti = FIX_MPY(wr, fi[j]) + FIX_MPY(wi, fr[j]);
				qr = fr[i];
				qi = fi[i];
				if (shift)
				{
					qr >>= 1;
					qi >>= 1;
				}
				fr[j] = qr - tr;
				fi[j] = qi - ti;
				fr[i] = qr + tr;
				fi[i] = qi + ti;
			}
		}
		--k;
		l = istep;
	}
	return scale;
}

//fft-related
short *m_f = NULL;
short *m_fr = NULL;
short *m_fi = NULL;
short m_bits = 10;

int pal[256];
int g_x = 0;
bool m_ctrl_pressed = false;

char m_filename[_MAX_PATH];
char *m_data = NULL;

float m_volume = 1.0f;

float m_oldVolume = 1.0f;

enum
{
	MODE_OSC = 0,
	MODE_FFT,
	MODE_FFTDB,
	MODE_ANALYZER,
	MODE_SONOGRAM
};

const char *modes[] = {
	"Waveform",
	"FFT",
	"FFT (db)",
	"Spectrum",
	"Sonogram"
};

int m_mode = MODE_ANALYZER;

float m_step = 1.0f;
float m_scale = 1.0f;
float m_maxamp = 0.0f;

int m_frames = 0;

HBITMAP g_hBitmap;
HDC g_hDC;
void *g_pBits = NULL;

int find_next_file(char *buffer, char *path, int, int step)
{
	char cwd[_MAX_PATH];
	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char filename[_MAX_FNAME];

	char next[_MAX_PATH];

#ifdef _DEBUG
	printf("Retrieving next file after: %s\n", path);
#endif

	strcpy(filename, "");
	strcpy(next, "");
	_getcwd(cwd, _MAX_PATH);

	//check - if no file specified, use cwd
	//else extract path from file

	if (path)
	{
		_fullpath(path_buffer, path, _MAX_PATH);
		_splitpath(path_buffer, drive, dir, fname, ext);
		_makepath(path_buffer, drive, dir, "", "");
		_makepath(filename, "", "", fname, ext);
	}
	else
		_getcwd(path_buffer, MAX_PATH);

#ifdef _DEBUG
	printf("path: %s\n", path_buffer);
#endif

	//get filelist
	_chdir(path_buffer);

	struct _finddata_t c_file;
	int hFile;

	int j = -1;
	int i = 0;

	if ((hFile = _findfirst("*.bmx", &c_file)) == -1L)
	{
#ifdef _DEBUG
		printf("No specified files in directory!\n");
#endif
	}
	else
	{
		do
		{
			const char *flag = "";

			if (strcmp(filename, c_file.name) == 0)
			{
				j = i;
				flag = "> ";
			}

			if ((i == 0) || (step > 0 && i == j + step) || (step < 0 && j <= 0))
				strcpy(next, c_file.name);

#ifdef _DEBUG
			printf("%s%s\n", flag, c_file.name);
#endif
			i++;

		}
		while (_findnext(hFile, &c_file) == 0);

		_findclose(hFile);
		_getcwd(path_buffer, MAX_PATH);	//remove slash if any

		strcat(path_buffer, "/");
		strcat(path_buffer, next);

		strcpy(next, path_buffer);
	}

	_chdir(cwd);

#ifdef _DEBUG
	printf("next: %s\n", next);
#endif

	strcpy(buffer, next);

	return 0;
}

int load(char *szName)
{
	FILE *fp;

	if (szName == NULL || strlen(szName) == 0)
		return 1;

	if (!(fp = fopen(szName, "rb")))
		return 1;

#ifdef _DEBUG
	printf("Loading %s\n", szName);
#endif

	long file_length = (int) _filelength(_fileno(fp));

	if (m_data != NULL)
		free(m_data);

	m_data = (char *) malloc(file_length);

	fread(m_data, file_length, 1, fp);
	fclose(fp);

	if (BmxLoad(m_data))
		return 1;

	m_maxamp = 0;

	return 0;
}

void get_next_file(int step)
{
	find_next_file(m_filename, m_filename, _MAX_PATH, step);
}

void OnLoad(HWND)
{
	char szFile[MAX_PATH] = { 0 };
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = "Buzz Tracker (*.bmx;*.bmw)\0*.bmx;*.bmw\0All Files\0*.*\0\0";
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFile = szFile;

	if (GetOpenFileName(&ofn))
	{
		strcpy(m_filename, ofn.lpstrFile);

		BmxStop();
		if (load(m_filename) == 0)
			BmxPlay();
	}
}

void CreateBmp(HWND hWnd)
{
	BITMAPINFO binfo;
	HDC hdc = GetDC(hWnd);
	binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	binfo.bmiHeader.biWidth = BMP_WIDTH;
	binfo.bmiHeader.biHeight = BMP_HEIGHT;
	binfo.bmiHeader.biPlanes = 1;
	binfo.bmiHeader.biBitCount = 32;
	binfo.bmiHeader.biCompression = 0;
	binfo.bmiHeader.biSizeImage = BMP_WIDTH * BMP_HEIGHT * 4;
	g_hBitmap = CreateDIBSection(hdc, &binfo, DIB_RGB_COLORS, &g_pBits, 0, 0);
	g_hDC = CreateCompatibleDC(hdc);
	SelectObject(g_hDC, g_hBitmap);
	ReleaseDC(hWnd, hdc);
}

void PutPixel(int x, int y, int color)
{
	if (x >= 0 && x < BMP_WIDTH && y >= 0 && y < BMP_HEIGHT)
		*((int *) g_pBits + x + y * BMP_WIDTH) = color;
}

void Line(int x1, int y1, int x2, int y2, int color)
{
	int d, ax, ay, sx, sy, dx, dy;
	dx = x2 - x1;
	ax = abs(dx) << 1;
	if (dx < 0)
		sx = -1;
	else
		sx = 1;
	dy = y2 - y1;
	ay = abs(dy) << 1;
	if (dy < 0)
		sy = -1;
	else
		sy = 1;
	PutPixel(x1, y1, color);
	if (ax > ay)
	{
		d = ay - (ax >> 1);
		while (x1 != x2)
		{
			if (d >= 0)
			{
				y1 += sy;
				d -= ax;
			}
			x1 += sx;
			d += ay;
			PutPixel(x1, y1, color);
		}
	}
	else
	{
		d = ax - (ay >> 1);
		while (y1 != y2)
		{
			if (d >= 0)
			{
				x1 += sx;
				d -= ay;
			}
			y1 += sy;
			d += ax;
			PutPixel(x1, y1, color);
		}
	}
}

void set_gradient_palette(int *palette, int i1, int i2, int c1, int c2)
{
	int i, k;
	int R, G, B;
	if (i2 == i1)
		k = 1;
	else
		k = i2 - i1;
	int R1, G1, B1, R2, G2, B2;
	R1 = (c1 >> 16) & 0xFF;
	G1 = (c1 >> 8) & 0xFF;
	B1 = (c1 >> 0) & 0xFF;
	R2 = (c2 >> 16) & 0xFF;
	G2 = (c2 >> 8) & 0xFF;
	B2 = (c2 >> 0) & 0xFF;
	for (i = i1; i <= i2; i++)
	{
		R = (R1 + (i - i1) * (R2 - R1) / k) & 0xFF;
		G = (G1 + (i - i1) * (G2 - G1) / k) & 0xFF;
		B = (B1 + (i - i1) * (B2 - B1) / k) & 0xFF;
		palette[i] = (0xff << 24) | (R << 16) | (G << 8) | B;
	}
}

float amp2db(float amp)
{
	float db = 20 * log10(amp);
	float norm_db = 1 + db / 90.0f;

	if (norm_db < 0)
		norm_db = 0;

	return norm_db;
}

void Paint()
{
	int w = BMP_WIDTH;
	int h = BMP_HEIGHT;
	int x0 = 0, y0 = 0;

	if (m_mode != MODE_SONOGRAM)
	{
		ZeroMemory(g_pBits, w * h * 4);
	}

	int pos = BmxGetPos();
	int *buf = (int *) bufs[0];

	int fft_size = 1 << m_bits;


	if (m_f == NULL)
	{
		init_fft(10);
		m_f = (short *) malloc(fft_size * sizeof(short));
		m_fr = (short *) malloc(fft_size * sizeof(short));
		m_fi = (short *) malloc(fft_size * sizeof(short));
	}

	float tmp = m_step;

	if (m_mode == MODE_ANALYZER)
		m_step = m_step * 4;

#define PUT_METHOD2

#ifdef PUT_METHOD1
	for (int i = 0; i < fft_size; i++)
	{
		int quad = buf[((int) (i * m_step) + pos) % WAVEOUTLENGTH];

		short l = quad & 0xffff;
		short r = quad >> 16;
		short amp = (l + r) / 2;

		m_fr[i] = amp;
		m_fi[i] = -amp;
	}
#else
	//even samples to real, odd to imaginary
	for (int i = 0; i < fft_size * 2; i++)
	{
		int quad = buf[((int) (i * m_step) + pos) % WAVEOUTLENGTH];

		short l = quad & 0xffff;
		short r = quad >> 16;
		short amp = (l + r) / 2;

		if (i & 1)
			m_fr[i / 2] = amp;
		else
			m_fi[i / 2] = amp;
	}
#endif

	m_step = tmp;

	//      fix_fftr(m_f, m_bits, 0);
	fix_fft(m_fr, m_fi, m_bits, 0);

	if (m_mode == MODE_ANALYZER)
		for (int i = 0; i < w; i++)
		{
			int x = i;

			int fft_ofs = 10;
			int fft_window = (int) (fft_size * 0.5f - fft_ofs);

			int j = fft_ofs + i * fft_window / w;

			float amp = sqrt((float) (m_fr[j] * m_fr[j] + m_fi[j] * m_fi[j])) / 32768.0f;

			amp *= m_scale;

			if (amp > m_maxamp)
				m_maxamp = amp;

			amp /= m_maxamp;

			int y = (int) (amp * h * m_scale);

			for (j = 0; j < y; j++)
			{
				int color = j * 255 / h;

				if (color < 0)
					color = 0;
				if (color > 255)
					color = 255;

				PutPixel(x, h - j, pal[color]);
			}
		}

	if ((m_mode == MODE_FFT) || (m_mode == MODE_FFTDB))
		for (int i = 0; i < w; i++)
		{
			int x = i;

			int j = i * (fft_size / 2) / w;

			float amp = sqrt((float) (m_fr[j] * m_fr[j] + m_fi[j] * m_fi[j])) / 32768.0f;

			if (m_mode == MODE_FFTDB)
				amp = amp2db(amp);
			else
				amp *= 4;

			int y = (int) (h - amp * h * m_scale);

			if (i != 0)
				Line(x, y, x0, y0, RGB(0, 255, 0));

			x0 = x;
			y0 = y;
		}

	if (m_mode == MODE_SONOGRAM)
		for (int i = 0; i < h; i++)
		{
			int j = i * (fft_size / 2) / w;

			float amp = sqrt((float) (m_fr[j] * m_fr[j] + m_fi[j] * m_fi[j])) / 32768.0f;

			amp = amp2db(amp);

			int color = (int) (amp * 255 * m_scale);

			if (color < 0)
				color = 0;
			if (color > 255)
				color = 255;

			PutPixel(g_x, h - i, pal[color]);
			PutPixel((g_x + 1) % w, h - i, RGB(255, 255, 255));
		}

	if (m_mode == MODE_OSC)
		for (int i = 0; i < w; i++)
		{
			int x = i;

			int quad = buf[((int) (i * m_step) + pos) % WAVEOUTLENGTH];

			short l = quad & 0xffff;
			short r = quad >> 16;
			short c = (l + r) / 2;

			int y = (int) (c * (h / 2) * m_scale / 32768.0f + h / 2);

			if (i != 0)
				Line(x, y, x0, y0, RGB(0, 255, 0));

			x0 = x;
			y0 = y;
		}

	m_frames++;
}

char szTitle[MAX_PATH];
void SetTitle(HWND hWnd)
{
	char buf[MAX_PATH];
	//sprintf (buf, "Bmxplayer - POS:%04d/%04d CPU:%4.1f%% step: %.2f fft: %d scale: %.2f\r", BmxCurrentTick, bmx.seq.songsize, BmxPerfomance, m_step, 1 << m_bits, m_scale);

	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath(m_filename, drive, dir, fname, ext);
	_makepath(path_buffer, "", "", fname, ext);

	if (m_ctrl_pressed)
		sprintf(buf, "Bmxplay %s - %s - step: %.2f scale: %.2f (maxamp: %.2f)", BMXPLAY_VERSION, modes[m_mode], m_step,
			m_scale, m_maxamp);
	else
		sprintf(buf, "Bmxplay %s - %s (%04d/%04d) - %s (CPU:%4.1f%%)", BMXPLAY_VERSION, path_buffer, BmxCurrentTick,
			(int) bmx.seq.songsize, modes[m_mode], BmxPerfomance);

	if (strcmp(buf, szTitle) != 0 && strlen(buf) != 0)
	{
		SetWindowText(hWnd, buf);
	}

	strcpy(szTitle, buf);
}

void nextSong(int step)
{
	get_next_file(step);
	BmxStop();

	if (load(m_filename) == 0)
		BmxPlay();

	m_maxamp = 0;
}

void restoreWindow(HWND hWnd)
{
	WINDOWPLACEMENT wp;
	GetWindowPlacement(hWnd, &wp);
	if (wp.showCmd != SW_MAXIMIZE)
	{

		int w = BMP_WIDTH;
		int h = BMP_HEIGHT;
		RECT rc;
		SetRect(&rc, 0, 0, w, h);

		AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE));

		w = rc.right - rc.left;
		h = rc.bottom - rc.top;

		GetWindowRect(hWnd, &m_RectDlg);

		SetWindowPos(hWnd, NULL, m_RectDlg.left, m_RectDlg.top, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
	}

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;

	m_ctrl_pressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

	switch (message)
	{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_LOAD:
					OnLoad(hWnd);
					break;

				case IDC_PAUSE:					
					m_pause = !m_pause;
					if (_taskbar) _taskbar->SetProgressState(hWnd, m_pause ? TBPF_PAUSED : TBPF_NORMAL);
					if (m_pause)
						BmxStop();
					else
						BmxPlay();

					break;
			}

			break;

		case WM_PAINT:

			if (!m_pause)
			{
				SetTitle(hWnd);

				if (_taskbar) _taskbar->SetProgressValue(hWnd, BmxCurrentTick, (int) bmx.seq.songsize);

				hdc = BeginPaint(hWnd, &ps);

				Paint();

				GetClientRect(hWnd, &rect);
				//SetStretchBltMode(hdc, HALFTONE);

				StretchBlt(hdc, 0, 0, rect.right, rect.bottom, g_hDC, 0, BMP_HEIGHT - 1, BMP_WIDTH, -BMP_HEIGHT,
					   SRCCOPY);

				EndPaint(hWnd, &ps);

				g_x++;
				g_x %= BMP_WIDTH;
			}
			break;

		case WM_KEYUP:

			switch (wParam)
			{
				case VK_NEXT:
				case VK_SPACE:
					nextSong(1);
					break;

				case VK_PRIOR:
					nextSong(-1);
					break;
			}
			break;

		case WM_KEYDOWN:
			if (m_ctrl_pressed)
				switch (wParam)
				{
					case VK_RETURN:
						m_step = 1.0f;
						m_scale = 1.0f;
						m_maxamp = 0.0f;
						restoreWindow(hWnd);
						break;

					case VK_RIGHT:
						if (m_step < WAVEOUTLENGTH / BMP_WIDTH / 2)
							m_step += 0.25;
						break;

					case VK_LEFT:
						if (m_step > 0.25)
							m_step -= 0.25;
						break;

					case VK_UP:
						if (m_scale < 20.0f)
							m_scale += 0.1f;
						m_maxamp = 0;
						break;

					case VK_DOWN:
						if (m_scale > 0.1f)
							m_scale -= 0.1f;
						m_maxamp = 0;
						break;
				}
			else
				switch (wParam)
				{
					case VK_F3: case 'O':
						PostMessage(hWnd, WM_COMMAND, IDC_LOAD, 0);
					break;

					case 'P':
						PostMessage(hWnd, WM_COMMAND, IDC_PAUSE, 0);
					break;


					case VK_ESCAPE:
						SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
						break;

					case VK_RETURN:
						m_mode++;
						if (m_mode > MODE_SONOGRAM)
							m_mode = MODE_OSC;
						g_x = 0;
						m_maxamp = 0;
						ZeroMemory(g_pBits, BMP_WIDTH * BMP_HEIGHT * 4);
						break;

					case VK_SUBTRACT:
					case VK_DOWN:
						m_volume = BmxGetVolume();
						CLAMP(m_volume, -0.025f, 0, 1);
						BmxSetVolume(m_volume);
						break;

					case VK_ADD:
					case VK_UP:
						m_volume = BmxGetVolume();
						CLAMP(m_volume, +0.025f, 0, 1);
						BmxSetVolume(m_volume);
						break;

					case VK_LEFT:
						BmxCurrentTick -= 4;
						if (BmxCurrentTick < 0)
							BmxCurrentTick = 0;
						//load_find_next_file(-1);
						//BmxCurrentTick = bmx.seq.songsize - 1;
						break;

					case VK_RIGHT:
						BmxCurrentTick += 4;
						if (BmxCurrentTick >= (int) bmx.seq.songsize)
							BmxCurrentTick = bmx.seq.songsize - 1;
						//load_find_next_file(1);
						//BmxCurrentTick = 0;
						break;

						/*
						   case VK_NEXT:
						   if (m_bits>1)
						   {
						   m_bits--;
						   if (m_f!=NULL) free(m_f);
						   m_f = NULL;
						   }
						   break;

						   case VK_PRIOR:
						   if (m_bits<14)
						   {
						   m_bits++;
						   if (m_f!=NULL) free(m_f);
						   m_f = NULL;
						   }
						   break;
						 */
				}
			break;

		case WM_CREATE:
			CreateBmp(hWnd);
			set_gradient_palette(pal, 0, 64, 0x0000ff, 0x00ff00);
			set_gradient_palette(pal, 64, 128, 0x00ff00, 0xff0000);
			set_gradient_palette(pal, 128, 192, 0xff0000, 0xffff00);
			set_gradient_palette(pal, 192, 255, 0xffff00, 0xffffff);
			DragAcceptFiles(hWnd, TRUE);
			break;

		case WM_DROPFILES:
		{

			HDROP hDrop = (HDROP) wParam;
			DragQueryFile(hDrop, 0, m_filename, MAX_PATH);
			DragFinish(hDrop);

			BmxStop();
			if (load(m_filename) == 0)
				BmxPlay();
		}
			break;

		case WM_TIMER:
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_MOUSEWHEEL:
		{
			m_volume = BmxGetVolume();
			float dv = -0.025f;
			if ((int) wParam > 0)
				dv *= -1;
			CLAMP(m_volume, dv, 0, 1);
			BmxSetVolume(m_volume);
		}
			break;

		case WM_LBUTTONDOWN:
			m_pressed = true;
			m_mx0 = m_mx;
			m_my0 = m_my;
			if (!m_captured)
			{
				WINDOWPLACEMENT wp;
				GetWindowPlacement(hWnd, &wp);
				if (wp.showCmd != SW_MAXIMIZE)
				{
					GetWindowRect(hWnd, &m_RectDlg);
					SetCapture(hWnd);
					m_captured = true;
					m_pt.x = m_mx;
					m_pt.y = m_my;
					ClientToScreen(hWnd, &m_pt);
				}
			}
			break;

		case WM_MBUTTONDOWN:
			m_mode++;
			if (m_mode > MODE_SONOGRAM)
				m_mode = MODE_OSC;
			g_x = 0;
			m_maxamp = 0;
			ZeroMemory(g_pBits, BMP_WIDTH * BMP_HEIGHT * 4);
			break;

		case WM_LBUTTONUP:
			m_pressed = false;
			if (m_captured)
			{
				ReleaseCapture();
				m_captured = false;
			}
			break;

		case WM_MOUSEMOVE:
			m_mx = (int) (short) LOWORD(lParam);
			m_my = (int) (short) HIWORD(lParam);
			if (m_captured)
			{
				POINT pt;
				pt.x = m_mx;
				pt.y = m_my;
				ClientToScreen(hWnd, &pt);
				SetWindowPos(hWnd, NULL, m_RectDlg.left + pt.x - m_pt.x, m_RectDlg.top + pt.y - m_pt.y,
					     m_RectDlg.right - m_RectDlg.left, m_RectDlg.bottom - m_RectDlg.top, SWP_SHOWWINDOW);
			}
			break;

		case WM_LBUTTONDBLCLK:
			nextSong(1);
			break;

		case WM_RBUTTONDBLCLK:
			nextSong(-1);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int main(int argc, char **argv)
{
	MSG msg;
	HWND hWnd;
	WNDCLASS wc;
	HINSTANCE hInstance = GetModuleHandle(NULL);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = (WNDPROC) WndProc;
	wc.hInstance = hInstance;
	wc.hbrBackground = NULL;
	wc.lpszClassName = "Bmxplayer";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	RegisterClass(&wc);

	int w = BMP_WIDTH;
	int h = BMP_HEIGHT;

	if (!
	    (hWnd =
	     CreateWindow(wc.lpszClassName, wc.lpszClassName, WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX,
			  0, 0, w, h, NULL, NULL, wc.hInstance, NULL)))
		return FALSE;

	RECT rc;

	SetRect(&rc, 0, 0, w, h);

	AdjustWindowRectEx(&rc, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL, GetWindowLong(hWnd, GWL_EXSTYLE));

	w = rc.right - rc.left;
	h = rc.bottom - rc.top;

	SetWindowPos(hWnd, NULL, (GetSystemMetrics(SM_CXSCREEN) - w) / 2, (GetSystemMetrics(SM_CYSCREEN) - h) / 2, w, h,
		     SWP_NOZORDER | SWP_NOACTIVATE);

	m_oldVolume = BmxGetVolume();

	// 'Progress'
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void **> (&(_taskbar)));
	if (SUCCEEDED(hr)) {
		if (FAILED(_taskbar->HrInit())) {
			_taskbar->Release();
			_taskbar = NULL;
		}
	} else {
		printf("[Win32TaskbarManager::init] Cannot create taskbar instance");
	}


//    m_filename[0] = 0;

	if (argc <= 1)
		find_next_file(m_filename, NULL, _MAX_PATH, 0);
	else
		strcpy(m_filename, *++argv);

	if (m_filename[0] != 0)
	{
		if (load(m_filename) == 0)
			BmxPlay();
	}
	else
		OnLoad(hWnd);

#ifdef USE_ACRYLIC
	DWORD color = 0x80ffffff;
	BOOL opaque = FALSE;

	if (HINSTANCE hDLL = LoadLibrary("dwmapi"))
		if (FARPROC pfn = GetProcAddress(hDLL, "DwmGetColorizationColor"))
			((HRESULT(__stdcall *)(DWORD *, BOOL *))pfn)(&color, &opaque);

	ACCENTPOLICY policy;
	policy.nAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
	policy.nColor = (color & 0xff00ff00) | ((color & 0xff) << 16) | ((color >> 16) & 0xff);
	policy.nFlags = 0;
	WINCOMATTRPDATA data;
	data.nAttribute = WCA_ACCENT_POLICY;
	data.ulDataSize = sizeof(policy);
	data.pData = &policy;

	if (HINSTANCE hDLL = LoadLibrary("user32"))
		if (FARPROC pfn = GetProcAddress(hDLL, "SetWindowCompositionAttribute"))
			((BOOL(WINAPI*)(HWND, WINCOMATTRPDATA*))pfn)(hWnd, &data);

#else // aero
	int p[4] = { -1 };
	if (HINSTANCE hDLL = LoadLibrary("dwmapi"))
		if (FARPROC pfn = GetProcAddress(hDLL, "DwmExtendFrameIntoClientArea"))
			((HRESULT(__stdcall *)(HWND, const int *))pfn)(hWnd, p);
#endif

	ShowWindow(hWnd, true);
	SetTimer(hWnd, 0, 20, NULL);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	BmxSetVolume(m_oldVolume);

	return (int) msg.wParam;
}
