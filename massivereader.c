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
    int port;
    int server_fd;

    char* prefix = NULL;
    int logFileDescriptor;
    fileNo = 0;
    newLog = 0;
    //int incomfd;

    read_parameters(argc, argv, &port, &prefix);

    logCreate(prefix, &logFileDescriptor);
    sigact();
    ////////////////////////////// Connect as server ////////////////////////


    int epoll_fd = create_epoll();
    server_fd = socket_bind(port, epoll_fd);


    struct epoll_event* events;

    events = (struct epoll_event *) malloc(MAXEVENTS * sizeof(struct epoll_event));



    printf("Przed while\n");

    int i = 1;
    while(i)
    {
        //printf("\nPrzed epoll_wait\n");

        int count = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        //printf("Jestem po epoll wait\n");


        for(int i = 0; i < count; ++i)
        {
            //printf("\nfor count : %d\n", count);
            //printf("Typ polaczenia: %d\n",(((struct typeOfConnection*)events[i].data.ptr)->type) );
            //printf("Deskryptor: %d\n",(((struct typeOfConnection*)events[i].data.ptr)->fd) );



            if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || !(events[i].events & EPOLLIN)) {
                printf("\n\nevents flag\n");
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
                    printf("epoll delete error main!\n");
                    //exit(-1);
                }
               /* if (close(((struct typeOfConnection *) events[i].data.ptr)->fd) == -1) {
                    printf("Cannot close file descriptor: %d", errno);
                    exit(-1);
                    //break;
                }*/
                /*  else
                  {
                      printf("Closed file descriptor: %d\n", events[i].data.fd);
                  }*/
                //epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
            }
            else if ((((struct typeOfConnection*)events[i].data.ptr)->type) == 1 ) //inet server
            {
                /*
                 * akceptuje połączenie AF_INET
                 */

                printf("Jestem w server inet\n");
                /*int incomfd = */
                acceptConnection((((struct typeOfConnection*)events[i].data.ptr)->fd), epoll_fd);
                printf("Serwer inet akceptuje polaczenie\n");

            }
            else if(((struct typeOfConnection*)events[i].data.ptr)->type == 2 ) //polączenie inet client
            {
                /*
                * czytam strukture z polączenie AF_INET
                * tworze połaczenie AF_LOCAL
                * odsyłam strukturę przez AF_INET
                */
                printf("czytanie struktury i polaczenie af_local, odeslanie struktury\n");
                read_from_inet_connection((((struct typeOfConnection*)events[i].data.ptr)->fd), epoll_fd);
            }
            else if((((struct typeOfConnection*)events[i].data.ptr)->type) == 3 ) //local
            {
                printf("Jestem local client\n\n");
                readFromLocalServer( ((struct typeOfConnection*)events[i].data.ptr), logFileDescriptor);



                /*char buf[256];
                read(((struct typeOfConnection*)events[i].data.ptr)->fd, buf, 21);
                write(1, buf, 21);*/


            }
        }
        if(newLog)
        {
            logCreate(prefix, &logFileDescriptor);
            newLog = 0;
        }

    }


    printf("END\n");

    close(server_fd);
    //close(incomfd);
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
    if( (connect(clientFd, (struct sockaddr *) address_local, sizeof(struct sockaddr_un))) == -1)
    {
        printf("massivereader - connectAsClient - error: %d\n", errno);
        return -1;
    }
    //set_non_blocking(clientFd);

    struct typeOfConnection* conn  = (struct typeOfConnection*) malloc (sizeof(struct typeOfConnection));
    conn->fd = clientFd;
    conn->type = 3;
    conn->address = *address_local;


    epollAdd1(EPOLLIN | EPOLLET, epoll_fd, conn);

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

    //write(1,&timestamp, 21);
    write(1,"\n",1);
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

/*
    write(1, "\n", 1);
    write(1, timestamp, 21);
    write(1, "\n", 1);
    write(1, strCurrTime, 21);
    write(1, "\n", 1);
*/

    /*write(1, "s: ", 3);
    write(1, &sendTime.tv_sec, sizeof(sendTime.tv_sec));
    write(1, "\n", 1);
    write(1, "ns: ", 4);
    write(1, &sendTime.tv_nsec, sizeof(sendTime.tv_nsec));
    write(1, "\n", 1);
*/

    //printf("%ld s  %ld ns\n", sendTime.tv_sec, sendTime.tv_nsec);


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

    //printf("%ld s  %ld ns\n", time->tv_sec, time->tv_nsec);


    char* buf = convertingTime(time);
    write(1,buf,21);
    //write(1,"\n",1);

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

    //*oldFd = newFd;
   /* if(close(*oldFd) == -1)
    {
        write(1, "close error\n",sizeof("close error\n"));
        exit(-1);
    }*/
   close(*oldFd);

    *oldFd = newFd;
    free(filePath);
}



////////////////////////////////////end klient local

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
        struct sockaddr_un address_local; //  = (struct sockaddr_un *) malloc(sizeof(struct sockaddr_un));
        //address_local = readStructure(fd);
        if( (read(fd, &address_local, sizeof(struct sockaddr_un))) != sizeof(struct sockaddr_un))
        {
            //write(1, &address_local, sizeof(struct sockaddr_un));
            //free(address_local);

            break;
        }

        printf("\n\nCzytam strukture\n\n");

        int clientFd = socketAsClient();
        if ((connectAsClient(clientFd, &address_local, epoll_fd)) != -1)
        {
            write(fd, &address_local, sizeof(struct sockaddr_un));
            //write(1, &address_local, sizeof(struct sockaddr_un));

            printf("\nConnected to local server\n");
        }
        else
        {
            address_local.sun_family = -1;
            write(fd, &address_local, sizeof(struct sockaddr_un));
            //write(1, &address_local, sizeof(struct sockaddr_un));

            printf("Cannot connect to local server!\n ");
        }
        //sleep(1);
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

    printf("accepted connection!\n");

    return incomfd;
}




struct sockaddr_un readStructure(int fd)
{
    struct sockaddr_un newadd;
    read(fd, &newadd, sizeof(struct sockaddr_un));

    write(1, newadd.sun_path, 108);

    return newadd;

}
////////////////////////////////////////////////////////////////end server inet

void epollAdd1(int flags, int epollFd, struct typeOfConnection* conn)
{
    struct epoll_event event;
    event.data.ptr = conn;
    event.events = flags;

    printf("epollAdd1 FD : %d\n", ((struct typeOfConnection*)event.data.ptr)->fd);

    if((epoll_ctl(epollFd, EPOLL_CTL_ADD, ((struct typeOfConnection*)event.data.ptr)->fd, &event)) == -1)
    {
        printf("epollAdd1 - epoll_ctl error!\n");
        exit(-1);
    }
}


void read_parameters(int argc, char** argv, int* port, char** prefix)
{
    int opt;
    int flags = 0;

    if(argc != 4)
    {
        printf("To few arguments!\n");
        exit(-1);
    }

    while( (opt = getopt(argc, argv, "O:")) != -1 )
    {
        switch(opt)
        {
            case 'O':
                *prefix = optarg;
                flags++;
                break;
        }
    }

    if(flags != 1)
    {
        printf("Enter the prefix!\n");
        exit(-1);
    }

    *port = strtol(argv[optind],NULL, 10);

    if(*port < 0 || *port > 64000)
    {
        printf("Wrong port number!\nChoose port between 0 and 64000\n");
        exit(-1);
    }

}
