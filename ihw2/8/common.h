#ifndef COMMON_H
#define COMMON_H

#define MAX_VISITORS 25
#define PAINTING_COUNT 5
#define MAX_VIEWERS 5
#define MAX_WAITING_TIME 2
#define MIN_WAITING_TIME 1

#define SHM_KEY_FILENAME "shmkey"
#define SEM_KEY_FILENAME "semkey"

typedef struct {
  volatile int _Atomic ready;
  volatile int _Atomic finish_flag;
} shared_mem_t;

#endif // COMMON_H