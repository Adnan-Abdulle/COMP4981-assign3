//
// Created by Adnan Abdulle on 2026-04-06.
//

#include "http_methods.h"
#include "http_methods_helpers.h"
#include <fcntl.h>
#include <ndbm.h>
enum { FILE_PERMS = 0644 };

static ssize_t readFile(int fd);

void handle_get(int client, const char *path) {
  ssize_t n;
  char fullPath[MAX_PATH];
  char fileBuff[MAX_FILE_SIZE];
  char header[MAX_HEADER];

  snprintf(fullPath, sizeof(fullPath), "test/%s", path + 1);

  int file = open(fullPath, O_RDONLY);
  if (file < 0) {
    file_not_found(client);
    return;
  }

  ssize_t fileSize = readFile(file);

  const char *type = get_content_type(path);

  snprintf(header, sizeof(header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %zd\r\n\r\n",
           type, fileSize);

  write_fully(client, header, strlen(header));

  while ((n = read(file, fileBuff, sizeof(fileBuff))) > 0) {
    write_fully(client, fileBuff, (size_t)n);
  }

  close(file);
}

void handle_head(int client, const char *path) {

  char header[MAX_HEADER];

  char fullPath[MAX_PATH];

  snprintf(fullPath, sizeof(fullPath), "test/%s", path + 1);

  int file = open(fullPath, O_RDONLY | O_CLOEXEC);
  if (file < 0) {
    file_not_found(client);
    return;
  }

  ssize_t fileSize = readFile(file);

  snprintf(header, sizeof(header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: %zd\r\n\r\n",
           fileSize);
  write_fully(client, header, strlen(header));

  close(file);
}

void handle_post(int client, char *path, char *body, size_t body_len) {
  char response[MAX_HEADER];
  DBM *db;

  datum key = {0};
  datum value = {0};

  db = dbm_open("test/postdata", O_RDWR | O_CREAT, FILE_PERMS);
  if (db == NULL) {
    internal_server_error(client);
    return;
  }

  key.dptr = path + 1;
  key.dsize = strlen(path + 1);

  value.dptr = body;
  value.dsize = body_len;

  if (dbm_store(db, key, value, DBM_REPLACE) != 0) {
    dbm_close(db);
    internal_server_error(client);
    return;
  }

  dbm_close(db);

  snprintf(response, sizeof(response),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: 14\r\n"
           "\r\n"
           "POST received\n");

  write_fully(client, response, strlen(response));
}

static ssize_t readFile(int fd) {

  if (fd == -1) {
    perror("Error: open failed");
    return -1;
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    perror("Error: fstat failed");
    return -1;
  }
  if (!S_ISREG(sb.st_mode)) {
    fprintf(stderr, "Error: This is not a regular file\n");
    return -1;
  }

  return sb.st_size;
}
