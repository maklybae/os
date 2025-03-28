#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

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
  int msqid_raw;
  if ((msqid_raw = msgget(KEY_RAW, IPC_CREAT | 0666)) < 0) {
    perror("msgget");
    return 1;
  }
  int msqid_processed;
  if ((msqid_processed = msgget(KEY_PROCESSED, IPC_CREAT | 0666)) < 0) {
    perror("msgget");
    return 1;
  }
  message_buf sbuf;
  message_buf rbuf;
  sbuf.mtype = 1;
  rbuf.mtype = 1;

  // Main logic
  int buffer_size = BUFFER_SIZE * 2; // my own heuristic
  char *buffer = (char *)malloc(buffer_size * sizeof(char));

  int count = 0, count_read = 0;
  while ((count_read = msgrcv(msqid_raw, &rbuf, BUFFER_SIZE, 1, 0)) != 0) {
    if (count_read < 0) {
      perror("read");
      return 1;
    }

    // Copy data to buffer
    memcpy(buffer + count, rbuf.mtext, count_read);
    count += count_read;

    // Process data
    int last_start = Process(buffer, count);

    if (last_start > 0) {
      // Write to channel
      int to_write = last_start, offset = 0;
      while (to_write > 0) {
        // Prepare message
        int buf_length = Min(to_write, BUFFER_SIZE);
        memcpy(sbuf.mtext, buffer + offset, buf_length);

        if (msgsnd(msqid_processed, &sbuf, buf_length, 0) < 0) {
          perror("msgsnd");
          return 1;
        }
        to_write -= buf_length;
        offset += buf_length;
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
    // Prepare message
    memcpy(sbuf.mtext, buffer + offset, Min(to_write, BUFFER_SIZE));
    size_t buf_length = Min(to_write, BUFFER_SIZE);

    if (msgsnd(msqid_processed, &sbuf, buf_length, 0) < 0) {
      perror("msgsnd");
      return 1;
    }
    to_write -= buf_length;
    offset += buf_length;
  }
  msgsnd(msqid_processed, &sbuf, 0, 0);

  free(buffer);
  return 0;
}