//
// Created by Adnan Abdulle on 2026-02-05.
//

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "socket_funcs.h"

int make_socket(void) {

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  return server_fd;
}

void bind_socket(int fd) {

  struct sockaddr_in addr;
  int addr_len = sizeof(addr);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(fd, (struct sockaddr *)&addr, (socklen_t)addr_len) == -1) {
    perror("bind");
    close(fd);
    exit(EXIT_FAILURE);
  }
}

void listen_socket(int fd) {

  if (listen(fd, SOMAXCONN) == -1) {
    perror("listen");
    close(fd);
    exit(EXIT_FAILURE);
  }
}

int accept_socket(int fd) {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int client = accept(fd, (struct sockaddr *)&client_addr, &client_len);

  if (client == -1) {
    perror("accept");
    return -1;
  }
  return client;
}
