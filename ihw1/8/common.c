#include "common.h"

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