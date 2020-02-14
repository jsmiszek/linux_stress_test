//
// Created by just on 04.02.2020.
//

#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>


int create_epoll()
{
    int epoll_fd;
    if((epoll_fd = epoll_create1(0)) == -1)
    {
        printf("Epoll_create1 error!\n");
        exit(-1);
    }
    return epoll_fd;
}


void set_non_blocking(int fd)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
    {
        printf("set_non_blocking fcntl-1 error!\n");
        exit(-1);
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        printf("set_non_blocking fcntl-2 error!\n");
        exit (-1);
    }
}


char* convertingTime(struct timespec tim)
{
    int minutes;
    int seconds;
    int nanoseconds;

    char* out = (char*)calloc(21, sizeof(char));
    out[20] = 0;

    seconds = (int)tim.tv_sec % 60;
    minutes = (int)tim.tv_sec % 3600 / 60;
    nanoseconds = (int)tim.tv_nsec;

    for(int i = 2; i != -1; i-- )
    {
        out[i] = (char)(minutes % 10) + '0';
        minutes /= 10;
    }

    out[3] = '*';
    out[4] = ':';

    for(int i = 6; i != 4; i-- )
    {
        out[i] = (char)(seconds % 10) + '0';
        seconds /= 10;
    }

    out[7] = ',';
    out[19] = '0' + (char)(nanoseconds % 10); nanoseconds /= 10;
    out[18] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[17] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[16] = '.';
    out[15] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[14] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[13] = '.';
    out[12] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[11] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[10] = '.';
    out[9] = (char)(nanoseconds % 10) + '0'; nanoseconds /= 10;
    out[8] = (char)(nanoseconds % 10) + '0';

    return out;
}