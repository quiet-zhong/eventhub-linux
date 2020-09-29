#include "dump.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <execinfo.h>     /* for backtrace() */

#define BACKTRACE_SIZE   128

void dump(char *filename, char *logname)
{
    const int MAX_STACK_FRAMES = 128;
    FILE* fd = fopen(logname, "wt+");
    if (NULL == fd)
        return;

    char buff[1024];
    char cmd[128] = "";
    int i;

    time_t t = time(NULL);
    struct tm* now = localtime(&t);
    sprintf(buff, "#########################################################\n"
                  "[%04d-%02d-%02d %02d:%02d:%02d]\n",
            now->tm_year + 1900,
            now->tm_mon + 1,
            now->tm_mday,
            now->tm_hour,
            now->tm_min,
            now->tm_sec);
    fwrite(buff, 1, strlen(buff), fd);

    void* array[MAX_STACK_FRAMES];
    int size = 0;
    char** strings = NULL;

    size = backtrace(array, MAX_STACK_FRAMES);
    strings = (char**)backtrace_symbols(array, size);
    for (i = 0; i < size; ++i)
    {
        printf("%s\n", strings[i]);

        sprintf(buff, "%d %s\n", i, strings[i]);
        fwrite(buff, 1, strlen(buff), fd);

        char *p1 = strstr(strings[i], "[");
        char *p2 = strstr(strings[i], "]");
        char address[64] = "";
        memcpy(address ,p1+1, p2 - p1 - 1);
        sprintf(cmd, "addr2line -e %s %s", filename, address);
        FILE *fPipe = popen(cmd, "r");
        if(fPipe != NULL)
        {
            memset(buff, 0, sizeof(buff));
            char* pData = fgets(buff, sizeof(buff), fPipe);
            pclose(fPipe);
            if(pData != NULL)
            {
                fwrite(pData, 1, strlen(pData), fd);
            }
        }
    }
    free(strings);
    fflush(fd);
    fclose(fd);
}
