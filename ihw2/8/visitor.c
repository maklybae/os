#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

enum PaintingFlags {
  PAINTING_1 = 1 << 0, // 00001
  PAINTING_2 = 1 << 1, // 00010
  PAINTING_3 = 1 << 2, // 00100
  PAINTING_4 = 1 << 3, // 01000
  PAINTING_5 = 1 << 4, // 10000
  ALL_PAINTINGS = 0x1F // 11111
};

void *raw_shmem = NULL;
int shm_id = -1;
int sem_id = -1;

void CleanUp() {
  if (shmdt(raw_shmem) < 0) {
    perror("shmdt");
  }
}

volatile sig_atomic_t _Atomic visitor_finish_flag = 0;
void handler_finish(int signum) { visitor_finish_flag = 1; }

int main() {
  // Signal handler
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Shared memory
  key_t shm_key = ftok(SHM_KEY_FILENAME, 1);
  if (shm_key < 0) {
    perror("ftok");
    return 1;
  }

  int mem_size = getpagesize();
  shm_id = shmget(shm_key, mem_size, 0);
  // Note: no IPC_CREAT
  if (shm_id < 0) {
    perror("shmget");
    return 1;
  }

  raw_shmem = shmat(shm_id, NULL, 0);
  if (!raw_shmem) {
    perror("shmmem");
    return 1;
  }
  shared_mem_t *shmem = (shared_mem_t *)raw_shmem;

  if (shmem->ready == 0) {
    // Shared memory is not ready
    fprintf(stderr, "Gallery is not open yet\n");
    CleanUp();
    return 1;
  }

  // Semaphores
  key_t sem_key = ftok(SEM_KEY_FILENAME, 1);
  if (sem_key == -1) {
    perror("ftok");
    CleanUp();
    return 1;
  }

  // PAINTING_COUNT for paintings + 1 for gallery
  if ((sem_id = semget(sem_key, PAINTING_COUNT + 1, 0)) < 0) {
    // Note: no IPC_CREAT
    perror("semget");
    CleanUp();
    return 1;
  }

  // Init random number generator
  srand(getpid());

  // Wait for gallery semaphore
  struct sembuf enter_arg[1] = {{.sem_num = 0, .sem_op = -1, .sem_flg = 0}};
  if (semop(sem_id, enter_arg, 1) < 0 && errno != EINTR) {
    perror("semop");
    CleanUp();
    return 1;
  }

  printf("Visitor %d entered the gallery\n", getpid());

  uint8_t visited = 0;
  while (visited != ALL_PAINTINGS && !shmem->finish_flag &&
         !visitor_finish_flag) {
    // Randomly select a painting to view
    // Ensure that the selected painting has not been viewed yet
    uint8_t next_painting;
    while (1 << (next_painting = rand() % PAINTING_COUNT) & visited) {
    }

    // Wait for painting semaphore
    struct sembuf paint_wait_arg[1] = {
        {.sem_num = next_painting + 1, .sem_op = -1, .sem_flg = 0}};
    if (semop(sem_id, paint_wait_arg, 1) < 0) {
      if (errno == EINTR) {
        // Interrupted by signal
        if (shmem->finish_flag || visitor_finish_flag) {
          break;
        }
        continue;
      }
      perror("semop");
      CleanUp();
      return 1;
    }

    printf("Visitor %d is viewing painting %d\n", getpid(), next_painting + 1);
    sleep(rand() % (MAX_WAITING_TIME - MIN_WAITING_TIME + 1) +
          MIN_WAITING_TIME);
    visited |= 1 << next_painting;

    // Signal painting semaphore
    struct sembuf paint_post_arg[1] = {
        {.sem_num = next_painting + 1, .sem_op = 1, .sem_flg = 0}};
    if (semop(sem_id, paint_post_arg, 1) < 0 && errno != EINTR) {
      perror("semop");
      return 1;
    }

    printf("Visitor %d finished viewing painting %d\n", getpid(),
           next_painting + 1);
  }

  // Signal gallery semaphore
  struct sembuf leave_arg[1] = {{.sem_num = 0, .sem_op = 1, .sem_flg = 0}};
  if (semop(sem_id, leave_arg, 1) < 0 && errno != EINTR) {
    perror("semop");
    CleanUp();
    return 1;
  }

  // Clean up
  CleanUp();
  printf("Visitor %d left the gallery\n", getpid());
  return 0;
}