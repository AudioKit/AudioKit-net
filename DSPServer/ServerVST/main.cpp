#include "stdafx.h"
#include <thread>
#include <stdio.h>
#include "TRACE.h"
#include "DSP_Server.h"

#include <string.h>
#define HAS_PREFIX(string, prefix) (strncmp(string, prefix, strlen(prefix)) == 0)

int main(int argc, char* argv[])
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    char cmd[100];

    MyDSP dsp;
    DSP_Server dspServer(&dsp);
    if (argc > 1)
    {
        sprintf(cmd, "load=%s", argv[1]);
        if (dspServer.command(cmd)) puts(cmd);
    }

    while (dspServer.isRunning())
    {
#if 1
        gets_s(cmd, 100);
        if (strlen(cmd) == 0) continue;

        if (HAS_PREFIX(cmd, "stop"))
        {
            dspServer.Stop();
        }
        else
        {
            //fprintf(stderr, "cmd: %s\n", cmd);
            if (dspServer.command(cmd)) puts(cmd);
        }
#else
        Sleep(20);
#endif
    }

    TRACE("Thread stopped: Quitting\n");
    return 0;
}
