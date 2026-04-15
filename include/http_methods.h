//
// Created by Adnan Abdulle on 2026-04-06.
//

#ifndef MAIN_HTTP_METHODS_H
#define MAIN_HTTP_METHODS_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "read_write.h"
#include "errors.h"

enum { MAX_PATH = 2048 };
enum { MAX_FILE_SIZE = 4096 };
enum { MAX_HEADER = 256 };

void handle_get(int client, const char *path);
void handle_head(int client, const char *path);
void handle_post(int client, char *path, char *body, size_t body_len);

#endif //MAIN_HTTP_METHODS_H
