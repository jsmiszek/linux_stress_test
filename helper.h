//
// Created by just on 04.02.2020.
//

#ifndef PROJEKT3_HELPER_H
#define PROJEKT3_HELPER_H

#include <time.h>

//int epoll_fd;

int create_epoll();
void set_non_blocking(int fd);
char* convertingTime(struct timespec tim);






#endif //PROJEKT3_HELPER_H
