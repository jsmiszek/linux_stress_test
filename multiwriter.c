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

int main(int argc, char** argv) {
    int numOfConnections;
    int port;
    float interval;
    float workTime;
    rejectedConnections = 0;
    acceptedConnections = 0;

    read_parameters(argc, argv, &numOfConnections, &port, &interval, &workTime);

    int *localFileDescriptors = (int *) calloc (numOfConnections, sizeof(int));
    int *fdTab = localFileDescriptors;

    printf("%d\n", numOfConnections);

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

    printf("Ide do while\n");

    //---------------------------------------------------------------------------------------------


    while (acceptedConnections + rejectedConnections < numOfConnections) {
        printf("Jestem w while1\n");
        int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        printf("count w while : %d\n", count);

        if (count == -1)
            printf("epoll_wait error\n");

        printf("Jestem w while2\n");


        for (int i = 0; i < count; i++) {
            printf("petla for : %d\n", i);
            printf("accepted Connection: %d\nrejected connecton: %d\n", acceptedConnections, rejectedConnections);
            printf("descriptor: %d\n", events[i].data.fd);

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN)) {
                /*if(events[i].data.fd != serverFd && events[i].data.fd != clientSocketFd)
                {
                    //remove_from_working_sockets(events[i].data.fd);
                }
                else*/
                {
                    /* if ((close(events[i].data.fd)) == -1)
                     {
                         printf("Multiwriter - main - close error %d \n", errno);
                         exit(-1);
                     }*/

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
                        printf("main - epoll_ctl delete error!\n");
                        exit(-1);
                    }

                    for(int i = 0; i < numOfConnections; ++i)
                        if(localFileDescriptors[i] == events[i].data.fd)
                            localFileDescriptors[i] = 0;

                    if (close(events[i].data.fd) == -1) {
                        printf("Cannot close descriptor: %d\n", events[i].data.fd);

                    }
                    printf("\n\nevents flag\n");/*
                    for (int j = 0; j < numOfConnections; ++j)
                    {
                        if( localFileDescriptors[j] == events[j].data.fd)
                        {
                            localFileDescriptors[j] = 0;
                        }
                    }*/

                }

            } else {
                if (events[i].data.fd == serverFd) {
                    /*int incomfd = */
                    acceptConnection(events[i].data.fd, epoll_fd, &fdTab);

                    /*
                     * Losuj deskryptor i wyslij znacznik!!!!!!
                     */

                    //char buf[BUFFER_SIZE];
//                    write(serverFd, "Jestem w polaczeniu local tu multiwriter\n", 30);
                    //write(1, &buf, 30);
                    printf("polaczylo sie z LOCAL- acceptConnection\n");

                } else if (events[i].data.fd == clientSocketFd) {
                    readFromServer(clientSocketFd);
                }
                // sleep(1);
            }


            //sleep(1);


        }
    }
    //sleep(1);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientSocketFd, NULL) < 0)
        printf("Cannot delete fd after while\n");

    close(clientSocketFd);

    createTimer(workTime);

    struct timespec tim;
    tim.tv_sec = (int)(interval * 1000) / 1000000000;
    tim.tv_nsec = (int)(interval * 1000) % 1000000000 ;


   /* printf("TABLICA DESKRYPTOROW! : ");
    for(int i = 0; i < acceptedConnections; i++)
        printf("  %d  ", localFileDescriptors[i]);
    printf("\n");*/
    int i = 1;
    while (i)
    {
       /* printf("Na deskryptor %d\n", localFileDescriptors[i]);
        write(localFileDescriptors[i], "ABCD\n", strlen("ABCD\n"));
        printf("Wyslalem komunikat po af local\n");*/

       sendDataToLocal(localFileDescriptors, local_address);
       nanosleep(&tim,NULL);

    }
   /* if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientSocketFd, NULL) < 0)
    {
        printf("after while cannot epoll_ctl delete!\n");
        exit(-1);
    }*/
    printf("Kończe program!!!\n");
    //close(serverFd);
    //tu wysyłam znaczniki
    //petla while ma sie skonczyc po ilosci polaczen udanych nie



    return 0;
}
/*

void closeLocalDescriptors(int fd)
{
    int i=0;
    while(i<local_sock_no)
        if(local_sock_fds[i++]==fd)
            break;
    if(i==local_sock_no)
        return;
    close(local_sock_fds[--i]);
    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,local_sock_fds[i],NULL);
    local_sock_fds[i]=local_sock_fds[--local_sock_no];
}
*/


void sendDataToLocal(int* fdTab, struct sockaddr_un address)
{
    struct timespec timestamp;
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
    write(1, timeRepr, 21);

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

    //write(1, out, 21);
    write(1,"\n", 1);

    return out;
}



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
        printf("Blad utworzenia budzika\n");
        exit(EXIT_FAILURE);
    }


    // zamiana z centy na nano
    double tempTime = workTime * 10000000;

    its.it_value.tv_sec = (int) (tempTime / 1000000000);
    its.it_value.tv_nsec = (long long) tempTime % 1000000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    // its.it_value.tv_sec = 2;
    //  its.it_value.tv_nsec = 999999999;

    if ( timer_settime(timer, 0, &its, NULL) == -1 )
    {
        printf("timer_settime error createTimer\n");
        printf("%d\n", errno);
        // errno 22 - wychodzi po za zakres
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

        printf("readFromServer!\n");

        if((int)local_address->sun_family == -1)
        {
            printf("readFromServer - send structure with -1 \n");
            rejectedConnections++;
        }
        else
        {
            printf("Odczytałem strukture sun_family = AF_LOCAL!\n");
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
    printf("Wysyłam %d razy strukture!\n", count);
}




int connectAsClient(int port)
{
    printf("Wchodze do connectAsClient\n");
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

    printf("wychodze z connect as client\n");

    return socket_fd;
}



struct sockaddr_un sockaddrRandom()
{

    struct sockaddr_un address;

    address.sun_family = AF_LOCAL;


    char* stream = (char*) calloc (108, sizeof(char));

    stream[0] = '\0';
    stream++;

    getrandom(stream, 107, GRND_NONBLOCK);

    strcpy(address.sun_path, stream);
    printf("Losowe\n");



    free(--stream);
    return address;
}


///////////////////////////////////////end klient


////////////////////////////////////////// start server


int createLocalServer(struct sockaddr_un address_local)
{
    printf("Wchodze do create local server\n");
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


    printf("wychodze z createLocalServer\n");

    return serverFd;
}




int acceptConnection(int serverFd, int epoll_fd, int** fdTab)
{
    /*struct sockaddr in_address;
    int in_address_size = sizeof(in_address);
    memset(&in_address, 0, sizeof(struct sockaddr_in));*/

    /*(struct sockaddr*) &in_address, (socklen_t *)&in_address_size)*/

    printf("acceptConnection WCHODZE KURWA!!\n");
    int incomfd = 0;

    if((incomfd = accept(serverFd, NULL, NULL)) == -1)
    {
        printf("acceptConnection error!\n");
        //rejectedConnections++;
        exit(-1);
    }
    //acceptedConnections++;

    //set_non_blocking(incomfd);


    **fdTab = incomfd;
    printf("acceptConnection deskryptor: %d\n",**fdTab);

    (*fdTab) += 1;

    epollAdd(incomfd, EPOLLIN | EPOLLET, epoll_fd);
    //printf("accepted local connection - acceptConnection!\n");

    return incomfd;
}
/*

void listenFromClient(int fd, int count)
{
    printf("Wchodzę w listen\n");
    if(listen(fd, count) == -1)
    {
        printf("multiwriter - listenFromClient - listen error\n");
        exit(-1);
    }
    printf("Wychodze z listen\n");
}
*/

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






