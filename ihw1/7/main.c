#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

const char FIFO_RAW_PATHNAME[] = "raw.fifo";
const char FIFO_PROCESSED_PATHNAME[] = "processed.fifo";

void MakeRawFifo() {
  int res = mknod(FIFO_RAW_PATHNAME, S_IFIFO | 0666, 0);
  if (res == -1 && errno != EEXIST) {
    perror("mknod");
    _exit(1);
  }
}

void MakeProcessedFifo() {
  int res = mknod(FIFO_PROCESSED_PATHNAME, S_IFIFO | 0666, 0);
  if (res == -1 && errno != EEXIST) {
    perror("mknod");
    _exit(1);
  }
}

void Reverse(char *arr, int from, int to) {
  while (from < to) {
    int tmp = arr[from];
    arr[from] = arr[to];
    arr[to] = tmp;
    from++;
    to--;
  }
}

void Process(char *arr, int size) {
  int start = 0;
  for (int i = 0; i < size; i++) {
    if (!('a' <= arr[i] && arr[i] <= 'z') &&
        !('A' <= arr[i] && arr[i] <= 'Z')) {
      Reverse(arr, start, i - 1);
      start = i + 1;
    }
  }
  Reverse(arr, start, size - 1);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <in> <out>\n", argv[0]);
    return 1;
  }

  pid_t cpid1 = fork();
  if (cpid1 < 0) {
    perror("fork1");
    return 1;
  }

  if (cpid1 == 0) {
    // ------------------------Child process - processor------------------

    // Open(create) FIFOs
    MakeRawFifo();
    MakeProcessedFifo();
    int fd_raw, fd_processed;
    if ((fd_raw = open(FIFO_RAW_PATHNAME, O_RDONLY)) < 0) {
      perror("open");
      return 1;
    }
    if ((fd_processed = open(FIFO_PROCESSED_PATHNAME, O_WRONLY)) < 0) {
      perror("open");
      return 1;
    }

    // Main logic
    char buffer[BUFFER_SIZE] = {};
    int res;
    if ((res = read(fd_raw, buffer, sizeof(buffer))) != 0) {
      if (res < 0) {
        perror("read");
        return 1;
      }

      // Process data
      Process(buffer, res);

      if (write(fd_processed, buffer, res) < 0) {
        perror("write");
        return 1;
      }
    }

    // Close used fd
    if (close(fd_raw) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_processed) < 0) {
      perror("close");
      return 1;
    }
  } else {
    //  ----------Parent process - reader/writer from/to file--------------

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
  }

  return 0;
}