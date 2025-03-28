#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 128
#define KEY_RAW 52
#define KEY_PROCESSED 53

typedef struct msgbuf {
  long mtype;
  char mtext[BUFFER_SIZE];
} message_buf;

#endif