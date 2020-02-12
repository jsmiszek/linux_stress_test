//
// Created by just on 04.02.2020.
//

#ifndef PROJEKT3_MULTIWRITER_H
#define PROJEKT3_MULTIWRITER_H

#include <time.h>

//int epoll_fd;
int rejectedConnections;
int acceptedConnections;

struct timespec sumTime;

int stop;

long long max;
long long min;



void read_parameters(int argc, char** argv, int* numOfConnections, int* port, float* interval, float* workTime);
int connectAsClient(int port);
struct sockaddr_un sockaddrRandom();
//void makeSocket(int* clientSocketFd);
//void listenFromClient(int fd, int count);
int acceptConnection(int serverFd, int epoll_fd, int** localFileDescriptors);

void sendStructureToServer(struct sockaddr_un address, int fd, int count);

void readFromServer(int fd);


/////////////////////server

int createLocalServer(struct sockaddr_un address_local);

void epollAdd(int fd, int flags, int epoll_fd);

//////////////////////////////////////// budzik

void createTimer(float workTime);

//////////////////////time

void sendDataToLocal(int* fdTab, struct sockaddr_un address);
void summaryTime(struct timespec startTime, struct timespec endTime);

///////////////////////////////////////signals

void sigHandler();
void sigact();


#endif //PROJEKT3_MULTIWRITER_H
