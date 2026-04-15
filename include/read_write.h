//
// Created by Adnan Abdulle on 2026-01-27.
//

#ifndef COMP_4981_ASSIGN1_READ_WRITE_H
#define COMP_4981_ASSIGN1_READ_WRITE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>

ssize_t write_fully(int fd, const void *buf, size_t n);
ssize_t read_fully(int fd, void *buf, size_t n);

#endif //COMP_4981_ASSIGN1_READ_WRITE_H
