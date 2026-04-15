//
// Created by Adnan Abdulle on 2026-02-08.
//

#include "errors.h"
enum { HEADER_SIZE = 256 };

void file_not_found(int client) {
  const char *body = "404 Not Found\n";

  char header[HEADER_SIZE];

  snprintf(header, sizeof(header),
           "HTTP/1.1 404 Not Found\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %zu\r\n"
           "\r\n",
           strlen(body));

  write_fully(client, header, strlen(header));
  write_fully(client, body, strlen(body));
}

void not_implemented(int client) {
  const char *errorHdr = "HTTP/1.1 501 Not Implemented\r\n"
                         "Content-Type: text/plain\r\n\r\n";

  const char *errorMsg = "501 Method not supported\n";

  write_fully(client, errorHdr, strlen(errorHdr));
  write_fully(client, errorMsg, strlen(errorMsg));
}

void bad_request(int client) {
  const char *errorHdr = "HTTP/1.1 400 Bad Request\r\n"
                         "Content-Type: text/plain\r\n\r\n";

  const char *errorMsg = "400 bad request\n";

  write_fully(client, errorHdr, strlen(errorHdr));
  write_fully(client, errorMsg, strlen(errorMsg));
}

void internal_server_error(int client) {
  const char *errorHdr = "HTTP/1.1 500 Internal Server Error\r\n"
                         "Content-Type: text/plain\r\n\r\n";

  const char *errorMsg = "500 internal server error\n";

  write_fully(client, errorHdr, strlen(errorHdr));
  write_fully(client, errorMsg, strlen(errorMsg));
}
