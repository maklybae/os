#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 5000

extern const char FIFO_RAW_PATHNAME[];
extern const char FIFO_PROCESSED_PATHNAME[];

void MakeRawFifo();
void MakeProcessedFifo();

#endif