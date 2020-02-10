//
// Created by just on 04.02.2020.
//

#ifndef PROJEKT3_MASSIVEREADER_H
#define PROJEKT3_MASSIVEREADER_H

#include <sys/un.h>


//int *epoll_fd;

struct typeOfConnection{
    int fd;
    int type;                       // 1-server     2-inet      3-local
    struct sockaddr_un address;
};
//////////////////////////////////server inet

int socket_bind(int port, int epoll_fd);
void listenToClient(int fd);

void read_from_inet_connection(int fd, int epoll_fd);

int acceptConnection(int server_fd, int epoll_fd);
struct sockaddr_un readStructure(int fd);

//////////////// klient ////////////////////////

int connectAsClient(int clientFd, struct sockaddr_un* address_local, int epoll_fd);
int socketAsClient();


/////////////////////////////else

void epollAdd1(int flags, int epollFd, struct typeOfConnection* conn);

void read_parameters(int argc, char** argv, int* port, char** prefix);


#endif //PROJEKT3_MASSIVEREADER_H

