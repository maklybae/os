#include <fcntl.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

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

  int fd_raw[2], fd_processed[2];
  pipe(fd_raw);
  pipe(fd_processed);

  pid_t cpid1 = fork();
  if (cpid1 < 0) {
    perror("fork1");
    return 1;
  }

  if (cpid1 == 0) {
    // ------------------------Child process - processor------------------

    // Close unused fd
    if (close(fd_raw[1]) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_processed[0]) < 0) {
      perror("close");
      return 1;
    }

    // Main logic
    char buffer[BUFFER_SIZE] = {};
    int res;
    if ((res = read(fd_raw[0], buffer, sizeof(buffer))) != 0) {
      if (res < 0) {
        perror("read");
        return 1;
      }

      // Process data
      Process(buffer, res);

      if (write(fd_processed[1], buffer, res) < 0) {
        perror("write");
        return 1;
      }
    }

    // Close used fd
    if (close(fd_raw[0]) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_processed[1]) < 0) {
      perror("close");
      return 1;
    }
  } else {
    //  ----------Parent process - reader/writer from/to file--------------

    // Close unused fd
    if (close(fd_raw[0]) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_processed[1]) < 0) {
      perror("close");
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

      if (write(fd_raw[1], buffer, res) < 0) {
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

    if ((res = read(fd_processed[0], buffer, sizeof(buffer))) != 0) {
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
    if (close(fd_to) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_from) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_raw[1]) < 0) {
      perror("close");
      return 1;
    }
    if (close(fd_processed[0]) < 0) {
      perror("close");
      return 1;
    }
  }

  return 0;
}