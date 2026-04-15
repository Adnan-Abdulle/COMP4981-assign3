//
// Created by Adnan Abdulle on 2026-02-05.
//

#ifndef COMP_4981_ASSIGN1_SOCKET_H
#define COMP_4981_ASSIGN1_SOCKET_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>


#define PORT 8080

int make_socket(void);
void bind_socket(int fd);
void listen_socket(int fd);
int accept_socket(int fd);


#endif //COMP_4981_ASSIGN1_SOCKET_H
