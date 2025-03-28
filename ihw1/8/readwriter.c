#include "common.h"

int main(int argc, char *argv[]) {
  // Open(create) FIFOs
  MakeRawFifo();
  MakeProcessedFifo();
  int fd_raw, fd_processed;
  if ((fd_raw = open(FIFO_RAW_PATHNAME, O_WRONLY)) < 0) {
    perror("open");
    return 1;
  }
  if ((fd_processed = open(FIFO_PROCESSED_PATHNAME, O_RDONLY)) < 0) {
    perror("open");
    return 1;
  }

  // Read logic
  char buffer[BUFFER_SIZE] = {};
  int fd_from = open(argv[1], O_RDONLY);
  if (fd_from < 0) {
    perror("open");
    return 1;
  }

  int res;
  if ((res = read(fd_from, buffer, sizeof(buffer))) != 0) {
    if (res < 0) {
      perror("read");
      return 1;
    }

    if (write(fd_raw, buffer, res) < 0) {
      perror("write");
      return 1;
    }
  }

  // Write logic
  int fd_to = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd_to < 0) {
    perror("open");
    return 1;
  }

  if ((res = read(fd_processed, buffer, sizeof(buffer))) != 0) {
    if (res < 0) {
      perror("read");
      return 1;
    }

    if (write(fd_to, buffer, res) < 0) {
      perror("write");
      return 1;
    }
  }

  // Close used fd
  if (close(fd_from) < 0) {
    perror("close");
    return 1;
  }
  if (close(fd_to) < 0) {
    perror("close");
    return 1;
  }
  if (close(fd_raw) < 0) {
    perror("close");
    return 1;
  }
  if (close(fd_processed) < 0) {
    perror("close");
    return 1;
  }

  return 0;
}
