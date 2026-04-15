//
// Created by Adnan Abdulle on 2026-01-27.
//

#include "read_write.h"
ssize_t write_fully(int fd, const void *buf, size_t n) {
  const unsigned char *p;
  size_t left;
  p = (const unsigned char *)buf;
  left = n;
  while (left > 0) {
    ssize_t w;
    w = write(fd, p, left);
    if (w > 0) {
      p += (size_t)w;
      left -= (size_t)w;
      continue;
    }
    if (w < 0 && errno == EINTR) {
      continue;
    }
    return -1;
  }
  return (ssize_t)n;
}

ssize_t read_fully(int fd, void *buf, size_t n) {
  unsigned char *p;
  size_t need;
  p = (unsigned char *)buf;
  need = n;
  while (need > 0) {
    ssize_t r;
    r = read(fd, p, need);
    if (r > 0) {
      p += (size_t)r;
      need -= (size_t)r;
      continue;
    }
    if (r == 0) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    return -1;
  }
  return (ssize_t)(n - need);
}
