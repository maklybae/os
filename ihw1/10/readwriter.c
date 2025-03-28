#include "common.h"
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>

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

  // Read logic
  char buffer[BUFFER_SIZE] = {};
  int fd_from = open(argv[1], O_RDONLY);
  if (fd_from < 0) {
    perror("open");
    return 1;
  }

  int res;
  while ((res = read(fd_from, buffer, BUFFER_SIZE)) != 0) {
    if (res < 0) {
      perror("read");
      return 1;
    }

    // Prepare message
    memcpy(sbuf.mtext, buffer, res);

    if (msgsnd(msqid_raw, &sbuf, res, 0) < 0) {
      perror("msgsnd");
      return 1;
    }
  }
  msgsnd(msqid_raw, &sbuf, 0, 0);

  // Write logic
  int fd_to = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd_to < 0) {
    perror("open");
    return 1;
  }

  while ((res = msgrcv(msqid_processed, &rbuf, BUFFER_SIZE, 1, 0)) != 0) {
    if (res < 0) {
      perror("msgrcv");
      return 1;
    }

    memcpy(buffer, rbuf.mtext, res);

    if (write(fd_to, buffer, res) < 0) {
      perror("write");
      return 1;
    }
  }

  return 0;
}
