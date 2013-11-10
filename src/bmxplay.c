#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include "bmxplay.h"

#ifdef _WIN32 // kbhit
#include <conio.h>
#else
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>

int _kbhit(void)
{
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if(ch != EOF)
        return 1;
    return 0;
}

void Sleep(int t)
{
    usleep(t * 1000);
}

#endif //kbhit

int main(int argc, char **argv)
{
    char *szName;
    FILE *fp;
    long file_length;
    char * data;
    int res;

    printf("Bmxplay version %s\n", BMXPLAY_VERSION);

    if (argc <= 1)
    {
        printf("Usage: bmxplay <filename.bmx>\n");
        return (0);
    }

    szName = *++argv;

    if (!(fp = fopen(szName, "rb")))
    {
        printf("Can't open file!\n");
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    data = (char *) malloc(file_length);
    fseek(fp, 0, SEEK_SET);
    if (fread(data, file_length, 1, fp)) {};
    fclose(fp);

    res = BmxLoad(data);
    if (res != 0)
    {
        printf("File \"%s\" has unsupported format (error code %d)\n", szName, res);
        return res;
    }

    BmxPlay();

    while (!_kbhit())
    {
        printf ("Playing %s (%04d/%04d)\r", szName, BmxCurrentTick, (int)bmx.seq.songsize);
        Sleep(10);
    }

    BmxStop();

    return 0;
}
