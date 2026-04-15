//
// Created by Adnan Abdulle on 2026-02-08.
//

#ifndef COMP_4981_ASSIGN1_ERRORS_H
#define COMP_4981_ASSIGN1_ERRORS_H
#include "read_write.h"
#include <string.h>

void file_not_found(int client);
void not_implemented(int client);
void bad_request(int client);
void internal_server_error(int client);
#endif //COMP_4981_ASSIGN1_ERRORS_H
