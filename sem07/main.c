#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#include "list.h"

void rm_shmid(int shmid) {
  if (shmctl(shmid, IPC_RMID, NULL) < 0) {
    perror("shmctl(IPC_RMID)");
    exit(1);
  }
}

int main(int argc, char const *argv[]) {
  key_t commonkey = ftok(".", 1);
  if (commonkey < 0) {
    perror("commonkey");
    return 1;
  }

  int mem_size = getpagesize();
  int shmid = shmget(commonkey, mem_size, IPC_CREAT | 0600);
  if (shmid < 0) {
    perror("shmget");
    return 1;
  }

  void *shmem = shmat(shmid, NULL, 0);
  if (!shmem) {
    perror("shmmem");
    return 1;
  }

  // Mark to delete shmid after shmdt
  rm_shmid(shmid);

  // Do something useful with shmem
  list_t *l = (struct list *)shmem;

  if (shmdt(shmem) < 0) {
    perror("shmdt");
    return 1;
  }

  return 0;
}