//BmxPlay project (c) 2001-2012 Joric^Proxium

//Bmxplay linux waveout driver

#define NUMBUFS 4		//quantity of buffers
#define BUFSIZE 4096		//buffer length in samples (samples per timeslice)

//////////////////////////////////////////////

#include <pthread.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

int BmxWorkBuffer(int samples, short *dest);

byte bufs[NUMBUFS][BUFSIZE * 4];

#define WAVEOUTLENGTH (BUFSIZE*NUMBUFS)

static volatile int playing = 0;

static void *thread_func(void *vptr_args)
{
	int i;

	int speed = 44100; 
	int samplesize = 16;
	int channels = 2; 
	int format = AFMT_S16_LE; 

	int fd = open("/dev/dsp", O_RDWR);

	if (fd < 0)
	{
		printf("Can't open /dev/dsp\n");
		_exit(1);
	}

	ioctl(fd, SNDCTL_DSP_SPEED, &speed);
	ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &samplesize);
	ioctl(fd, SNDCTL_DSP_CHANNELS, &channels);
	ioctl(fd, SNDCTL_DSP_SETFMT, &format);
	
	//pre-render
	for (i = 0; i < NUMBUFS; i++)
		BmxWorkBuffer(BUFSIZE, (short *) bufs[i]);

	//play
	playing = 1;
	while (playing)
	{
		for (i = 0; i < NUMBUFS; i++)
		{
			if (write(fd, bufs[i], BUFSIZE * 4)) 
			{
				//some error handling
			}

			BmxWorkBuffer(BUFSIZE, (short *) bufs[i]);
		}
	}

	close(fd);

	//cleanup
	for (i = 0; i < NUMBUFS; i++)
		memset(bufs[i], 0, BUFSIZE * 4);

	return 0;
}


void BmxPlay(void)
{
	pthread_t thread;
	pthread_create(&thread, NULL, thread_func, NULL);
}

void BmxStop(void)
{
	playing = 0;
}

