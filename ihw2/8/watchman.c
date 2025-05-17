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

shared_mem_t *shmem = NULL;

void handler_finish(int signum) {
  if (shmem) {
    shmem->finish_flag = 1;
  } else {
    // Shmem is not initialized
    _exit(0);
  }
}

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
  int shm_id = shmget(shm_key, mem_size, IPC_CREAT | IPC_EXCL | 0600);
  if (shm_id < 0) {
    perror("shmget");
    return 1;
  }

  void *raw_shmem = shmat(shm_id, NULL, 0);
  if (!raw_shmem) {
    perror("shmmem");
    return 1;
  }
  shmem = (shared_mem_t *)raw_shmem;

  // Semaphores
  key_t sem_key = ftok(SEM_KEY_FILENAME, 1);
  if (sem_key == -1) {
    perror("ftok");
    return 1;
  }

  int sem_id;
  // PAINTING_COUNT for paintings + 1 for gallery
  if ((sem_id = semget(sem_key, PAINTING_COUNT + 1,
                       IPC_CREAT | IPC_EXCL | 0600)) < 0) {
    perror("semget");
    return 1;
  }

  // Initialize semaphores
  unsigned short values[] = {MAX_VISITORS, MAX_VIEWERS, MAX_VIEWERS,
                             MAX_VIEWERS,  MAX_VIEWERS, MAX_VIEWERS};
  if (semctl(sem_id, 0, SETALL, values) < 0) {
    perror("semctl SETALL");
    return 1;
  }

  // Set ready flag (gallery is open)
  shmem->ready = 1;
  printf("Working day started...\n");

  // Waiting for close signal
  while (!shmem->finish_flag) {
    sched_yield();
  }

  // Clean up
  if (shmdt(raw_shmem) < 0) {
    perror("shmdt");
    return 1;
  }
  if (shmctl(shm_id, IPC_RMID, NULL) < 0) {
    perror("shmctl(IPC_RMID)");
    return 1;
  }
  if (semctl(sem_id, 0, IPC_RMID) < 0) {
    perror("semctl IPC_RMID");
    return 1;
  }

  printf("Working day finished...\n");
  return 0;
}