//
// Created by Adnan Abdulle on 2026-04-14.
//

#include "errors.h"
#include "http_methods.h"
#include "request_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum { MAX_METHOD = 16 };

static ssize_t get_content_length_dynamic(const char *buff) {
    const char *content_length_header;
    int content_length;

    content_length_header = strstr(buff, "Content-Length:");
    if (content_length_header == NULL) {
        return -1;
    }

    if (sscanf(content_length_header, "Content-Length: %d", &content_length) != 1) {
        return -1;
    }

    if (content_length < 0) {
        return -1;
    }

    return (ssize_t)content_length;
}

void handle_request_dynamic(int client, const char *buff) {
    char method[MAX_METHOD];
    char path[MAX_PATH];

    if (sscanf(buff, "%15s %2047s", method, path) != 2) {
        bad_request(client);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        handle_get(client, path);
    } else if (strcmp(method, "HEAD") == 0) {
        handle_head(client, path);
    } else if (strcmp(method, "POST") == 0) {
        char *body = strstr(buff, "\r\n\r\n");
        if (body == NULL) {
            bad_request(client);
            return;
        }

        body += 4;

        ssize_t content_length = get_content_length_dynamic(buff);
        if (content_length < 0) {
            bad_request(client);
            return;
        }

        handle_post(client, path, body, (size_t)content_length);
    } else {
        not_implemented(client);
    }
}
