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

#define MAX_EVENTS 50
#define BUFFER_SIZE 256

int main(int argc, char** argv)
{
    int numOfConnections;
    int port;
    float interval;
    float workTime;
    rejectedConnections = 0;
    acceptedConnections = 0;

    read_parameters(argc, argv, &numOfConnections, &port, &interval, &workTime);
    int *localFileDescriptors = (int *) malloc (numOfConnections * sizeof(int));

    printf("%d\n", numOfConnections);

    int epoll_fd = create_epoll();


    //------------------ server ------------------------------------


    struct sockaddr_un local_address = sockaddrRandom();
    int serverFd;
    serverFd = createLocalServer(local_address);
    epollAdd(serverFd, EPOLLIN | EPOLLET, epoll_fd);
    listen(serverFd, numOfConnections);


    // -------------------------- client ---------------------------------------------------------


    int clientSocketFd;
    clientSocketFd = connectAsClient(port);
    epollAdd(clientSocketFd, EPOLLIN | EPOLLET, epoll_fd);

    sendStructureToServer(local_address, clientSocketFd, numOfConnections);

    //---------------------------------------------------------------------------------------------

    struct epoll_event* events;
    events = (struct epoll_event *) malloc (MAX_EVENTS * sizeof(struct epoll_event));

    printf("Ide do while\n");

    //---------------------------------------------------------------------------------------------


    while(acceptedConnections + rejectedConnections < numOfConnections)
    {
        printf("Jestem w while1\n");
        printf("accepted Connection: %d\nrejected %d\n",acceptedConnections, rejectedConnections);
        int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (count == -1)
            printf("epoll_wait error\n");

        printf("Jestem w while2\n");


        for (int i = 0; i < count; i++) {
            printf("petla for : %d\n", i);

            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN))
            {
                /*if(events[i].data.fd != serverFd && events[i].data.fd != clientSocketFd)
                {
                    close(events[i].data.fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd,NULL);
                    //remove_from_working_sockets(events[i].data.fd);
                }
                else*/
                {
                    if ((close(events[i].data.fd)) == -1)
                    {
                        printf("Multiwriter - main - close error %d \n", errno);
                        exit(-1);
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    printf("\n\nevents flag\n");
                }

            }
            else
            {
                if (events[i].data.fd == serverFd)
                {
                    /*int incomfd = */
                    acceptConnection(events[i].data.fd, epoll_fd, &localFileDescriptors);

                    /*
                     * Losuj deskryptor i wyslij znacznik!!!!!!
                     */

                    //char buf[BUFFER_SIZE];
                    write(serverFd, "Jestem w polaczeniu local tu multiwriter\n", 30);
                    //write(1, &buf, 30);
                    printf("polaczylo sie z LOCAL- acceptConnection\n");

                }
                else if (events[i].data.fd == clientSocketFd)
                {
                    readFromServer(clientSocketFd);
                }
                sleep(1);
            }


            sleep(1);


        }
    }
    //tu wysyłam znaczniki
    //petla while ma sie skonczyc po ilosci polaczen udanych nie



    return 0;
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
            printf("Odczytałem strukture sockaddr_un z 1!\n");
            acceptedConnections++;
        }
        sleep(1);
    }
}


void sendStructureToServer(struct sockaddr_un address, int fd, int count)
{
    for(int i = 0; i < count; i++)
    {
        write(fd, &address, sizeof(address));
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

    if( (bind(serverFd, (struct sockaddr*) &address_local, sizeof(address_local))) == -1 )
    {
        printf("multiwriter - createLocalServer - bind error: %d\n", errno);
        exit(-1);
    }
    set_non_blocking(serverFd);


    printf("wychodze z createLocalServer\n");

    return serverFd;
}




int acceptConnection(int serverFd, int epoll_fd, int** localFileDecriptors)
{
    /*struct sockaddr in_address;
    int in_address_size = sizeof(in_address);
    memset(&in_address, 0, sizeof(struct sockaddr_in));*/

    /*(struct sockaddr*) &in_address, (socklen_t *)&in_address_size)*/

    int incomfd;

    if((incomfd = accept(serverFd, NULL, NULL)) == -1)
    {
        printf("acceptConnection error!\n");
        exit(-1);
    }

    set_non_blocking(incomfd);
    epollAdd(incomfd, EPOLLIN | EPOLLET, epoll_fd);

    **localFileDecriptors = incomfd;
    (*localFileDecriptors)++;

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







