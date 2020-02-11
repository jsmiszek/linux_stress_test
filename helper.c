//
// Created by just on 04.02.2020.
//

#include "helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <fcntl.h>


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

