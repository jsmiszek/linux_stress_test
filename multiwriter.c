//
// Created by just on 03.02.2020.
//

#include "multiwriter.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/random.h>
#include <errno.h>
#include <signal.h>

#define MAX_EVENTS 50
#define BUFFER_SIZE 256
#define mld 1000000000

int main(int argc, char** argv) {

    printf("I'm working...\n\n");
    int numOfConnections;
    int port;
    float interval;
    float workTime;
    rejectedConnections = 0;
    acceptedConnections = 0;
    stop = 1;
    sumTime.tv_sec = 0;
    sumTime.tv_nsec = 0;

    min = 1000000000;
    max = 0;

    read_parameters(argc, argv, &numOfConnections, &port, &interval, &workTime);

    int *localFileDescriptors = (int *) calloc (numOfConnections, sizeof(int));
    int *fdTab = localFileDescriptors;

    sigact();
    int epoll_fd = create_epoll();


    //------------------ server ------------------------------------


    struct sockaddr_un local_address = sockaddrRandom();
    int serverFd;
    serverFd = createLocalServer(local_address);
    epollAdd(serverFd, EPOLLIN | EPOLLET, epoll_fd);
    if (listen(serverFd, numOfConnections) == -1) {
        printf("Listen server error\n");
        exit(-1);
    }


    // -------------------------- client ---------------------------------------------------------


    int clientSocketFd;
    clientSocketFd = connectAsClient(port);
    epollAdd(clientSocketFd, EPOLLIN | EPOLLET, epoll_fd);

    sendStructureToServer(local_address, clientSocketFd, numOfConnections);

    //---------------------------------------------------------------------------------------------

    struct epoll_event *events;
    events = (struct epoll_event *) malloc(MAX_EVENTS * sizeof(struct epoll_event));

    //---------------------------------------------------------------------------------------------


    while (acceptedConnections + rejectedConnections < numOfConnections) {

        int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (count == -1)
            printf("epoll_wait error\n");


        for (int i = 0; i < count; i++) {

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {

                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);

                for(int i = 0; i < numOfConnections; ++i)
                {
                    if (localFileDescriptors[i] == events[i].data.fd)
                        localFileDescriptors[i] = 0;
                }
                close(events[i].data.fd);

            } else {
                if (events[i].data.fd == serverFd)
                {
                    acceptConnection(events[i].data.fd, epoll_fd, &fdTab);

                } else if (events[i].data.fd == clientSocketFd)
                {
                    readFromServer(clientSocketFd);
                }
            }
        }
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientSocketFd, NULL);

    close(clientSocketFd);

    createTimer(workTime);

    struct timespec time;
    time.tv_sec = (int)(interval * 1000) / 1000000000;
    time.tv_nsec = (int)(interval * 1000) % 1000000000 ;


    while(stop)
    {
       sendDataToLocal(localFileDescriptors, local_address);
       nanosleep(&time,NULL);
    }

    printf("Writing time: %ld s   %ld ns\n", sumTime.tv_sec, sumTime.tv_nsec);
    printf("Minimum: \t%lld ns\n", min);
    printf("Maximum: \t%lld ns\n\n", max);

    return 0;
}


void sendDataToLocal(int* fdTab, struct sockaddr_un address)
{
    struct timespec timestamp;
    struct timespec timestampEnd;
    //struct timespec* sum;
    int randSocketNum;
    char* timeRepr;

    if(clock_gettime(CLOCK_REALTIME, &timestamp) == -1)
    {
        printf("sendLocalData - clock_gettime error %d\n", errno);
        exit(-1);
    }

    randSocketNum = rand() % (acceptedConnections);
    while(fdTab[randSocketNum] == 0)
        randSocketNum = rand() % (acceptedConnections);

    timeRepr = convertingTime(timestamp);

    if(write(fdTab[randSocketNum], timeRepr, 21) == -1 )
    {
        write(1, "write1 error\n", 14);
        exit(-1);
    }
    if(write(fdTab[randSocketNum], &address.sun_path, 108) == -1 )
    {
        write(1, "write2 error\n", 14);
        exit(-1);
    }
    if(write(fdTab[randSocketNum], &timestamp, sizeof(timestamp)) == -1 )
    {
        write(1, "write2 error\n", 14);
        exit(-1);
    }


    if(clock_gettime(CLOCK_REALTIME, &timestampEnd) == -1)
    {
        printf("sendLocalData - clock_gettime2 error %d\n", errno);
        exit(-1);
    }
    summaryTime(timestamp, timestampEnd);

    free(timeRepr);

}



void summaryTime(struct timespec start, struct timespec end)
{
   long long nanosecond = 0;
   long long second = 0;

   nanosecond = end.tv_nsec - start.tv_nsec;
   second = end.tv_sec - start.tv_sec;

   if(nanosecond < 0)
   {
       nanosecond = mld + nanosecond;
       second -= 1;
   }

   sumTime.tv_sec += second;

   if( (sumTime.tv_nsec + nanosecond) >= mld )
   {
       sumTime.tv_nsec = (sumTime.tv_nsec + nanosecond) % mld;
       sumTime.tv_sec += 1;
   }
   else
   {
       sumTime.tv_nsec += nanosecond;
   }

   if(nanosecond < min)
        min = nanosecond;
   if(nanosecond > max)
       max = nanosecond;

}

////////////////////////////// signal

void sigHandler()
{
    stop = 0;
}

void sigact()
{
    struct sigaction sigact;
    sigact.sa_flags = 0;
    sigact.sa_handler = sigHandler;
    if(sigaction(SIGUSR1, &sigact, NULL) == -1)
    {
        write(1, "Sigact error\n", sizeof("Sigact error\n"));
        exit(-1);
    }
}

///////////////////////////////////timer


void createTimer(float workTime)
{
    timer_t timer;
    struct itimerspec its;

    struct sigevent sigeven;
    sigeven.sigev_notify = SIGEV_SIGNAL;
    sigeven.sigev_signo = SIGUSR1;
    sigeven.sigev_value.sival_ptr = &timer;

    if ( timer_create(CLOCK_REALTIME, &sigeven, &timer) == -1)
    {
        write(1, "timer error\n", sizeof("timer error\n"));
        exit(-1);
    }

    // centy -> nano
    double tempTime = workTime * 10000000;

    its.it_value.tv_sec = (int) (tempTime / 1000000000);
    its.it_value.tv_nsec = (long long) tempTime % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;


    if ( timer_settime(timer, 0, &its, NULL) == -1 )
    {
        write(1, "Settime error\n",sizeof("Settime error\n"));
        exit(-1);
    }
}


void readFromServer(int fd)
{
    while(1)
    {

        struct sockaddr_un * local_address = (struct sockaddr_un *) malloc (sizeof(struct sockaddr_un));

        if( (read(fd, local_address, sizeof(struct sockaddr_un))) != sizeof(struct sockaddr_un) )
            break;

        if((int)local_address->sun_family == -1)
        {
            rejectedConnections++;
        }
        else
        {
            acceptedConnections++;
        }
    }
}


void sendStructureToServer(struct sockaddr_un address, int fd, int count)
{
    for(int i = 0; i < count; i++)
    {
        if( write(fd, &address, sizeof(struct sockaddr_un)) == -1)
        {
            printf("sendStructureToServer - write - error!\n");
            exit(-1);
        }
    }
}


int connectAsClient(int port)
{
    int socket_fd;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_fd == -1)
    {
        printf("Socket error!\n");
        exit(-1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_aton("127.0.0.1", (struct in_addr *) &address.sin_addr.s_addr);
    bzero(&(address.sin_zero), 8);

    if((connect(socket_fd, (const struct sockaddr*) &address, sizeof(address))) == -1)
    {
        printf("multiwriter - connectAsClient - Connect error!\n");
        exit(-1);
    }
    set_non_blocking(socket_fd);


    return socket_fd;
}


struct sockaddr_un sockaddrRandom()
{

    struct sockaddr_un address;

    address.sun_family = AF_LOCAL;

    char* stream = (char*) calloc (108, sizeof(char));

    stream[0] = '\0';
    //stream++;

    getrandom(stream + 1, 106, GRND_NONBLOCK);

    strcpy(address.sun_path, stream);

    free(stream);
    return address;
}


///////////////////////////////////////end klient


////////////////////////////////////////// start server


int createLocalServer(struct sockaddr_un address_local)
{
    int serverFd;
    serverFd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if(serverFd == -1)
    {
        printf("multiwriter - connectAsServer - socket error!\n");
        exit(-1);
    }

    if( (bind(serverFd, (struct sockaddr*) &address_local, sizeof(struct sockaddr_un))) == -1 )
    {
        printf("multiwriter - createLocalServer - bind error: %d\n", errno);
        exit(-1);
    }
    set_non_blocking(serverFd);

    return serverFd;
}



int acceptConnection(int serverFd, int epoll_fd, int** fdTab)
{

    int incomfd = 0;

    if((incomfd = accept(serverFd, NULL, NULL)) == -1)
    {
        printf("acceptConnection error!\n");
        exit(-1);
    }
    //set_non_blocking(incomfd);

    **fdTab = incomfd;

    (*fdTab) += 1;

    epollAdd(incomfd, EPOLLIN | EPOLLET, epoll_fd);

    return incomfd;
}
////////////////////////////////////////end


/////////////////////////////////////// else
void read_parameters(int argc, char** argv, int* numOfConnections,int* port, float* interval, float* workTime)
{
    int opt;
    int flagToS = 0;
    int flagToP = 0;
    int flagToD = 0;
    int flagToT = 0;

    if(argc != 9)
    {
        printf("To few arguments!\n");
        exit(-1);
    }

    while( (opt = getopt(argc, argv, "S:p:d:T:")) != -1 )
    {
        switch (opt)
        {
            case 'S':
                *numOfConnections = strtol(optarg, NULL, 10);
                flagToS++;
                break;
            case 'p':
                *port = strtol(optarg, NULL, 10);
                flagToP++;
                break;
            case 'd':
                *interval = strtof(optarg, NULL);
                flagToD++;
                break;
            case 'T':
                *workTime = strtof(optarg, NULL);
                flagToT++;
                break;
        }
    }

    if( !flagToD || !flagToP || !flagToS || !flagToT )
    {
        printf("Enter ./program -S <int> -p <int> -d <float> -T <float>\n");
        exit(-1);
    }


    if(*port < 0 || *port > 64000)
    {
        printf("Wrong port number!\nChoose port between 0 and 64000\n");
        exit(-1);
    }

}



void epollAdd(int fd, int flags, int epoll_fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = flags;

    if((epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event)) == -1)
    {
        printf("epollAdd - epoll_ctl error!\n");
        exit(-1);
    }
}






