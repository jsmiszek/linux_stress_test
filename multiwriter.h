//
// Created by just on 04.02.2020.
//

#ifndef PROJEKT3_MULTIWRITER_H
#define PROJEKT3_MULTIWRITER_H

//int epoll_fd;
int rejectedConnections;
int acceptedConnections;

void read_parameters(int argc, char** argv, int* numOfConnections, int* port, float* interval, float* workTime);
int connectAsClient(int port);
struct sockaddr_un sockaddrRandom();
//void makeSocket(int* clientSocketFd);
//void listenFromClient(int fd, int count);
int acceptConnection(int serverFd, int epoll_fd, int** localFileDecriptors);

void sendStructureToServer(struct sockaddr_un address, int fd, int count);

void readFromServer(int fd);


/////////////////////server

int createLocalServer(struct sockaddr_un address_local);






#endif //PROJEKT3_MULTIWRITER_H
