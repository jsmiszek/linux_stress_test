//
// Created by just on 03.02.2020.
//

#include "massivereader.h"
#include "helper.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <errno.h>
#include <time.h>
#include <signal.h>


#define MAXEVENTS 32
#define BUFSIZE 128


int main(int argc, char** argv)
{
    printf("I'm working...\n");
    int port;
    int server_fd;

    char* prefix = NULL;
    int logFileDescriptor;
    fileNo = 0;
    newLog = 0;

    read_parameters(argv, &prefix, &port, argc);

    logCreate(prefix, &logFileDescriptor);
    sigact();


    int epoll_fd = create_epoll();
    server_fd = socket_bind(port, epoll_fd);


    struct epoll_event* events;

    events = (struct epoll_event *) malloc(MAXEVENTS * sizeof(struct epoll_event));


    int i = 1;
    while(i)
    {
        int count = epoll_wait(epoll_fd, events, MAXEVENTS, -1);

        for(int i = 0; i < count; ++i)
        {

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
            }
            else if ((((struct typeOfConnection*)events[i].data.ptr)->type) == 1 ) //inet server
                acceptConnection((((struct typeOfConnection*)events[i].data.ptr)->fd), epoll_fd);

            else if(((struct typeOfConnection*)events[i].data.ptr)->type == 2 ) //polączenie inet client
                read_from_inet_connection((((struct typeOfConnection*)events[i].data.ptr)->fd), epoll_fd);

            else if((((struct typeOfConnection*)events[i].data.ptr)->type) == 3 ) //local
                readFromLocalServer( ((struct typeOfConnection*)events[i].data.ptr), logFileDescriptor);
        }
        if(newLog)
        {
            logCreate(prefix, &logFileDescriptor);
            newLog = 0;
        }
    }

    close(server_fd);
    free(events);
    return 0;
}

///////////////////////////////klient


int socketAsClient()
{
    int clientFd = socket(AF_LOCAL, SOCK_STREAM, 0);

    if(clientFd == -1)
    {
        printf("massivereader - connectAsClient - socket error!\n");
        exit(-1);
    }

    return clientFd;
}

int connectAsClient(int clientFd, struct sockaddr_un* address_local, int epoll_fd)
{

    if(connectClient(clientFd, address_local) == -1)
        return -1;

    struct typeOfConnection* conn  = (struct typeOfConnection*) malloc (sizeof(struct typeOfConnection));
    conn->fd = clientFd;
    conn->type = 3;
    conn->address = *address_local;

    epollAdd1(EPOLLIN | EPOLLET, epoll_fd, conn);

    return 0;
}

int connectClient(int clientFd, struct sockaddr_un* address_local)
{
    if( (connect(clientFd, (struct sockaddr *) address_local, sizeof(struct sockaddr_un))) == -1)
    {
        printf("massivereader - connectAsClient - error: %d\n", errno);
        return -1;
    }
    return 0;
}

void readFromLocalServer(struct typeOfConnection* conn, int logFileDescriptor)
{
    char timestamp[21];
    char address[108];
    struct timespec sendTime;
    struct timespec currTime;
    char* strCurrTime;
    int fd = conn->fd;
    char* diffTime;

    if(read(fd, &timestamp, 21) == -1)
    {
        write(1, "read1 error\n", 13);
        exit(-1);
    }

    if(read(fd, &address, 108) == -1)
    {
        write(1, "read2 error\n", 13);
        exit(-1);
    }
    if(read(fd, &sendTime, sizeof(struct timespec)) == -1)
    {
        write(1, "read1 error\n", 13);
        exit(-1);
    }

    if( strcmp(address, conn->address.sun_path) != 0 )
        return;

    if(clock_gettime(CLOCK_REALTIME, &currTime) == -1)
    {
        printf("sendLocalData - clock_gettime error %d\n", errno);
        exit(-1);
    }

    strCurrTime = convertingTime(currTime);


    if(write(logFileDescriptor, strCurrTime, 20) == -1)
    {
        write(1, "write1 error\n", 14);
        exit(-1);
    }
    if(write(logFileDescriptor, " : ", 3) == -1)
    {
        write(1, "write2 error\n", 14);
        exit(-1);
    }
    if(write(logFileDescriptor, timestamp, 20) == -1)
    {
        write(1, "write3 error\n", 14);
        exit(-1);
    }
    if(write(logFileDescriptor, " : ", 3) == -1)
    {
        write(1, "write4 error\n", 14);
        exit(-1);
    }

    diffTime = timeDelay(sendTime, currTime);

    if(write(logFileDescriptor, diffTime, 20) == -1)
    {
        write(1, "write4 error\n", 14);
        exit(-1);
    }
    if(write(logFileDescriptor, "\n", 1) == -1)
    {
        write(1, "write4 error\n", 14);
        exit(-1);
    }

    free(diffTime);
}



char* timeDelay(struct timespec sendTime, struct timespec currTime)
{
    struct timespec time;
    long long first;
    long long second;
    long long result;

    first = (sendTime.tv_sec * 1000000000l) + sendTime.tv_nsec;
    second = (currTime.tv_sec * 1000000000l) + currTime.tv_nsec;
    result = second - first;
    time.tv_sec = result / 1000000000l;
    time.tv_nsec = result % 1000000000l;

    char* buf = convertingTime(time);

    return buf;
}

void logCreate(char* prefix, int* oldFd)
{
    int newFd = -1;
    char* filePath = (char*) calloc (strlen(prefix) + 4, sizeof(char));

    while(newFd == -1)
    {
        sprintf(filePath, "%s%03d", prefix, fileNo++);
        newFd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }

   close(*oldFd);

    *oldFd = newFd;
    free(filePath);
}


////////////////////////////////////end klient local

//////////////////////////////////// signal

void sigHandler()
{
    newLog = 1;
}

void sigact()
{
    struct sigaction sigact;
    sigact.sa_flags = 0;
    sigact.sa_handler = sigHandler;
    if(sigaction(SIGUSR1, &sigact, NULL) == -1)
    {
        write(1, "sigact error\n", sizeof("sigact error\n"));
        exit(-1);
    }
}

///////////////////////////////////////// server inet

void read_from_inet_connection(int fd, int epoll_fd)
{
    while(1)
    {
        struct sockaddr_un address_local;
        if( (read(fd, &address_local, sizeof(struct sockaddr_un))) != sizeof(struct sockaddr_un))
            break;

        int clientFd = socketAsClient();
        if ((connectAsClient(clientFd, &address_local, epoll_fd)) != -1)
        {
            write(fd, &address_local, sizeof(struct sockaddr_un));
        }
        else
        {
            address_local.sun_family = -1;
            write(fd, &address_local, sizeof(struct sockaddr_un));
            printf("Cannot connect to local server!\n ");
        }
    }
}


void listenToClient(int fd)
{
    if (listen(fd, 5) == -1)
    {
        printf("Listen error!\n");
        exit(-1);
    }
}


int socket_bind(int port, int epoll_fd)
{
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
        printf("Socket error!\n");
        exit(-1);
    }

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton("127.0.0.1", (struct in_addr *) &server_addr.sin_addr.s_addr);
    bzero(&(server_addr.sin_zero), 8);

    if((bind(server_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1)
    {
        printf("massivereader - Bind error!\n");
        exit(-1);
    }

    set_non_blocking(server_fd);

    struct typeOfConnection* conn = (struct typeOfConnection*) malloc (sizeof(struct typeOfConnection));
    conn->fd = server_fd;
    conn->type = 1;

    epollAdd1(EPOLLIN | EPOLLET, epoll_fd, conn);
    listenToClient(server_fd);

    return server_fd;

}


int acceptConnection(int server_fd, int epoll_fd)
{
    struct sockaddr_in in_address;
    socklen_t in_address_size = sizeof(in_address);

    int incomfd;

    if((incomfd = accept(server_fd,(struct sockaddr*) &in_address, &in_address_size)) == -1)
    {
        printf("acceptConnection error!\n");
        exit(-1);
    }

    set_non_blocking(incomfd);

    struct typeOfConnection* conn  = (struct typeOfConnection*) calloc (1, sizeof(struct typeOfConnection));
    conn->fd = incomfd;
    conn->type = 2;

    epollAdd1(EPOLLIN | EPOLLET, epoll_fd, conn);

    return incomfd;
}



////////////////////////////////////////////////////////////////end server inet

void epollAdd1(int flags, int epollFd, struct typeOfConnection* conn)
{
    struct epoll_event event;
    event.data.ptr = conn;
    event.events = flags;

    if((epoll_ctl(epollFd, EPOLL_CTL_ADD, ((struct typeOfConnection*)event.data.ptr)->fd, &event)) == -1)
    {
        printf("epollAdd1 - epoll_ctl error!\n");
        exit(-1);
    }
}


void read_parameters(char** argv, char** prefix, int* port, int argc)
{
    if(argc != 4)
    {
        printf("To few arguments!\n");
        exit(-1);
    }

    int flags = 0;
    int opt;

    while( (opt = getopt(argc, argv, "O:")) != -1 )
    {
        switch(opt)
            case 'O':
                *prefix = optarg;
                flags++;
                break;
    }

    *port = strtol(argv[optind],NULL, 10);

    if(*port < 0 || *port > 64000)
    {
        printf("Wrong port number!\nChoose port between 0 and 64000\n");
        exit(-1);
    }

    if(flags != 1)
    {
        printf("Enter the prefix!\n");
        exit(-1);
    }

}
