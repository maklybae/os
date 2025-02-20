#include <fcntl.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <argument1> <argument2>\n", argv[0]);
    return 1;
  }

  int fd_from = open(argv[1], O_RDONLY);
  if (fd_from < 0) {
    perror("open");
    return 1;
  }

  int fd_to;
  if (access(argv[1], X_OK) == 0) {
    fd_to = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0777);
  } else {
    fd_to = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }
  if (fd_to < 0) {
    perror("open");
    return 1;
  }

  char buffer[256] = {};
  int res;
  while ((res = read(fd_from, buffer, sizeof(buffer))) != 0) {
    if (res < 0) {
      perror("read");
      return 1;
    }

    if (write(fd_to, buffer, res) < 0) {
      perror("write");
      return 1;
    }
  }

  close(fd_from);
  close(fd_to);
}