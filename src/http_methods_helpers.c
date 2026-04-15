//
// Created by Adnan Abdulle on 2026-04-14.
//

#include "http_methods_helpers.h"
#include <string.h>

const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');

    if (!ext)
        return "application/octet-stream";

    if (strcmp(ext, ".html") == 0)
        return "text/html";
    if (strcmp(ext, ".txt") == 0)
        return "text/plain";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".gif") == 0)
        return "image/gif";

    return "application/octet-stream";
}
