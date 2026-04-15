#include "errors.h"
#include "socket_funcs.h"
#include <dlfcn.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum { MAX_METHOD = 16 };
enum { MAX_REQUEST = 1024 };
enum { MAX_CHILD_POOL = 5 };

typedef struct {
  pid_t pid;
  int fd;
} child;

typedef void (*request_handler_fn)(int, const char *);

static void poll_loop(int fd, child child_fds[MAX_CHILD_POOL]);
static void start_server(void);
static void create_child_pool(child child_fds[MAX_CHILD_POOL]);
static void restart_child(child child_fds[MAX_CHILD_POOL], int index);
static void monitor_children(child child_fds[MAX_CHILD_POOL]);
static void send_fd(int socket, int fd_to_send);
static _Noreturn void child_worker(int worker_fd, int index);
static int recv_fd(int socket);
static time_t get_library_mtime(const char *lib_path);
static void load_request_library(const char *lib_path, void **lib_handle,
                                 request_handler_fn *handler,
                                 time_t *loaded_mtime);

static void handle_sigchld(int sig) { (void)sig; }

static time_t get_library_mtime(const char *lib_path) {
  struct stat sb;

  if (stat(lib_path, &sb) < 0) {
    return (time_t)-1;
  }

  return sb.st_mtime;
}

static void load_request_library(const char *lib_path, void **lib_handle,
                                 request_handler_fn *handler,
                                 time_t *loaded_mtime) {
  time_t new_mtime = get_library_mtime(lib_path);

  if (new_mtime == (time_t)-1) {
    fprintf(stderr, "stat failed for %s\n", lib_path);
    *handler = NULL;
    return;
  }

  if (*lib_handle != NULL && *loaded_mtime == new_mtime) {
    return;
  }

  if (*lib_handle != NULL) {
    dlclose(*lib_handle);
    *lib_handle = NULL;
    *handler = NULL;
  }

  *lib_handle = dlopen(lib_path, RTLD_NOW);
  if (*lib_handle == NULL) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
    *handler = NULL;
    return;
  }

  const char *ignored_error = dlerror();
  (void)ignored_error;

  *handler = (request_handler_fn)dlsym(*lib_handle, "handle_request_dynamic");

  const char *error = dlerror();
  if (error != NULL) {
    fprintf(stderr, "dlsym failed: %s\n", error);
    dlclose(*lib_handle);
    *lib_handle = NULL;
    *handler = NULL;
    return;
  }

  *loaded_mtime = new_mtime;
}

static _Noreturn void child_worker(int worker_fd, int index) {
  const char *lib_path = "./librequest_handler.so";
  void *lib_handle = NULL;
  request_handler_fn handler = NULL;
  time_t loaded_mtime = 0;

  load_request_library(lib_path, &lib_handle, &handler, &loaded_mtime);

  while (1) {
    int client_fd = recv_fd(worker_fd);
    printf("Child %d received client fd %d\n", index, client_fd);
    fflush(stdout);

    char buff[MAX_REQUEST];
    ssize_t n = read(client_fd, buff, sizeof(buff) - 1);

    if (n <= 0) {
      shutdown(client_fd, SHUT_WR);
      close(client_fd);
      continue;
    }

    buff[n] = '\0';

    load_request_library(lib_path, &lib_handle, &handler, &loaded_mtime);

    if (handler == NULL) {
      internal_server_error(client_fd);
      shutdown(client_fd, SHUT_WR);
      close(client_fd);
      continue;
    }

    handler(client_fd, buff);

    shutdown(client_fd, SHUT_WR);
    close(client_fd);
  }
}

static void monitor_children(child child_fds[MAX_CHILD_POOL]) {
  int status;
  pid_t dead_pid;

  while ((dead_pid = waitpid(-1, &status, WNOHANG)) > 0) {
    printf("Child with PID %d died\n", dead_pid);
    fflush(stdout);

    for (int i = 0; i < MAX_CHILD_POOL; i++) {
      if (child_fds[i].pid == dead_pid) {
        printf("Restarting child at index %d\n", i);
        fflush(stdout);

        restart_child(child_fds, i);
        break;
      }
    }
  }
}

static void restart_child(child child_fds[MAX_CHILD_POOL], int index) {
  int sv[2];

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
    perror("socketpair");
    exit(EXIT_FAILURE);
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    close(sv[0]);

    printf("Restarted child %d PID: %d\n", index, getpid());
    fflush(stdout);

    child_worker(sv[1], index);
    exit(EXIT_SUCCESS);
  }

  close(sv[1]);
  close(child_fds[index].fd);

  child_fds[index].fd = sv[0];
  child_fds[index].pid = pid;
}

static void create_child_pool(child child_fds[MAX_CHILD_POOL]) {

  for (int i = 0; i < MAX_CHILD_POOL; i++) {
    int sv[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
      perror("socketpair");
      exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if (pid == 0) {

      close(sv[0]);
      printf("Child %d PID: %d\n", i, getpid());
      fflush(stdout);
      child_worker(sv[1], i);
      exit(EXIT_SUCCESS);

    } else {

      close(sv[1]);
      child_fds[i].fd = sv[0];
      child_fds[i].pid = pid;
    }
  }
}

static void send_fd(int socket, int fd_to_send) {
  struct msghdr msg = {0};
  struct iovec io = {0};
  char buf[1] = {'F'}; // dummy data

  io.iov_base = buf;
  io.iov_len = sizeof(buf);

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;

  char cmsgbuf[CMSG_SPACE(sizeof(int))];
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  if (cmsg == NULL) {
    fprintf(stderr, "CMSG_FIRSTHDR failed\n");
    exit(EXIT_FAILURE);
  }
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));

  *((int *)CMSG_DATA(cmsg)) = fd_to_send;

  msg.msg_controllen = sizeof(cmsgbuf);

  if (sendmsg(socket, &msg, 0) < 0) {
    perror("sendmsg");
    exit(EXIT_FAILURE);
  }
}

static int recv_fd(int socket) {
  struct msghdr msg;
  struct iovec io;
  struct cmsghdr *cmsg;
  char buf[1];
  char cmsgbuf[CMSG_SPACE(sizeof(int))];
  int received_fd;
  ssize_t bytes_received;

  memset(&msg, 0, sizeof(msg));
  memset(&io, 0, sizeof(io));
  memset(cmsgbuf, 0, sizeof(cmsgbuf));

  io.iov_base = buf;
  io.iov_len = sizeof(buf);

  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = cmsgbuf;
  msg.msg_controllen = sizeof(cmsgbuf);

  bytes_received = recvmsg(socket, &msg, 0);
  if (bytes_received < 0) {
    perror("recvmsg");
    exit(EXIT_FAILURE);
  }

  if (bytes_received == 0) {
    fprintf(stderr, "recvmsg: connection closed\n");
    exit(EXIT_FAILURE);
  }

  cmsg = CMSG_FIRSTHDR(&msg);
  if (cmsg == NULL) {
    fprintf(stderr, "No control message received\n");
    exit(EXIT_FAILURE);
  }

  if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
    fprintf(stderr, "Invalid control message\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(received_fd));
  return received_fd;
}

static void poll_loop(int fd, child child_fds[MAX_CHILD_POOL]) {
  struct pollfd poll_fds[1];
  int client;
  int next_child = 0;

  poll_fds[0].fd = fd;
  poll_fds[0].events = POLLIN;

  while (1) {
    monitor_children(child_fds);

    if (poll(poll_fds, 1, -1) < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("poll failed");
      return;
    }

    if (poll_fds[0].revents & POLLIN) {
      client = accept_socket(fd);
      if (client == -1) {
        if (errno == EINTR) {
          continue;
        }
        perror("accept");
        continue;
      }

      printf("\nClient connected\n");
      fflush(stdout);

      if (child_fds[next_child].pid > 0) {
        send_fd(child_fds[next_child].fd, client);

        printf("fd sent to child index %d with PID %d\n", next_child,
               child_fds[next_child].pid);
        fflush(stdout);
      }

      close(client);
      next_child = (next_child + 1) % MAX_CHILD_POOL;
    }
  }
}

static void start_server(void) {
  int fd = make_socket();
  bind_socket(fd);
  listen_socket(fd);

  printf("Server successfully created!!\n");

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigchld;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGCHLD, &sa, NULL);
  child children[MAX_CHILD_POOL];
  create_child_pool(children);
  poll_loop(fd, children);
}

int main(void) {

  start_server();
  return 0;
}
