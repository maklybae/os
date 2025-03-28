#include "common.h"
#include <stdlib.h>

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

  return 0;
}