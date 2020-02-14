//
// Created by just on 04.02.2020.
//

#ifndef PROJEKT3_MASSIVEREADER_H
#define PROJEKT3_MASSIVEREADER_H

#include <sys/un.h>
#include <time.h>


int fileNo;
int newLog;

struct typeOfConnection{
    int fd;
    int type;                       // 1-server     2-inet      3-local
    struct sockaddr_un address;
};
//////////////////////////////////server inet

int socket_bind(int port, int epoll_fd);
void listenToClient(int fd);

void read_from_inet_connection(int fd, int epoll_fd);

void addConnection(int server_fd, int epoll_fd);
int acceptConnection(int fd);


//////////////// klient ////////////////////////

int connectAsClient(int clientFd, struct sockaddr_un* address_local, int epoll_fd);
int connectClient(int clientFd, struct sockaddr_un* address_local);

int socketAsClient();
void readFromLocalServer(struct typeOfConnection* conn, int logFileDescriptor);

char* timeDelay(struct timespec sendTime, struct timespec currTime);

void logCreate(char* prefix, int* oldFd);

/////////////////////////////else

void epollAdd1(int flags, int epollFd, struct typeOfConnection* conn);

void read_parameters(char** argv, char** prefix, int* port, int argc);

///////////////////////// signal

void sigHandler();
void sigact();


#endif //PROJEKT3_MASSIVEREADER_H

