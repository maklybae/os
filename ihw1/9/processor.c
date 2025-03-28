#include "common.h"

#include <stdio.h>
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

int Process(char *arr, int size) {
  int start = 0;
  for (int i = 0; i < size; i++) {
    if (!('a' <= arr[i] && arr[i] <= 'z') &&
        !('A' <= arr[i] && arr[i] <= 'Z')) {
      Reverse(arr, start, i - 1);
      start = i + 1;
    }
  }
  return start; // return the start of the last word
}

int Min(int a, int b) { return a < b ? a : b; }

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
  int buffer_size = BUFFER_SIZE * 2; // my own heuristic
  char *buffer = (char *)malloc(buffer_size * sizeof(char));

  int count = 0, count_read = 0;
  while ((count_read = read(fd_raw, buffer + count, BUFFER_SIZE)) != 0) {
    if (count_read < 0) {
      perror("read");
      return 1;
    }
    // printf("read %d bytes\n", count_read); // TO DELETE
    count += count_read;

    // Process data
    int last_start = Process(buffer, count);

    if (last_start > 0) {
      // Write to channel
      int to_write = last_start, offset = 0;
      while (to_write > 0) {
        int written =
            write(fd_processed, buffer + offset, Min(to_write, BUFFER_SIZE));
        if (written != Min(to_write, BUFFER_SIZE)) {
          perror("write");
          return 1;
        }
        to_write -= written;
        offset += written;
      }

      // Move the last word to the beginning of the buffer
      for (int i = last_start; i < count; i++) {
        buffer[i - last_start] = buffer[i];
      }
      count -= last_start;
    }

    if (buffer_size - count < BUFFER_SIZE) {
      buffer_size += BUFFER_SIZE * 2; // my own heuristic
      buffer = (char *)realloc(buffer, buffer_size * sizeof(char));
    }
  }

  // Reverse and write the last word
  Reverse(buffer, 0, count - 1);
  int to_write = count, offset = 0;
  while (to_write > 0) {
    int written =
        write(fd_processed, buffer + offset, Min(to_write, BUFFER_SIZE));
    if (written != Min(to_write, BUFFER_SIZE)) {
      perror("write");
      return 1;
    }
    to_write -= written;
    offset += written;
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

  free(buffer);
  return 0;
}