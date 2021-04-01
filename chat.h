#ifndef CHAT_H
#define CHAT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#define PORT 7000
#define EPOLL_SIZE 5000
#define BUF_SIZE 0xFFFF

#endif // CHAT_H
