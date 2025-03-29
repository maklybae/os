#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

volatile sig_atomic_t _Atomic finish_flag = 0;
void handler_finish(int signum) { finish_flag = 1; }

void rm_shmid(int shmid) {
  if (shmctl(shmid, IPC_RMID, NULL) < 0) {
    perror("shmctl(IPC_RMID)");
    exit(1);
  }
}

int main(int argc, char const *argv[]) {
  // Достаточно выполнить один раз IPC_RMID (в одном процессе), так как
  // сегмент Shared Memory будет удален, когда все процессы его освободят.
  // Будем выполнять IPC_RMID в процессе, который создал сегмент Shared Memory.
  bool should_remove = false;

  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  key_t commonkey = ftok(".", 1);
  if (commonkey < 0) {
    perror("ftok");
    return 1;
  }

  volatile atomic_int *counter = NULL;

  int mem_size = getpagesize();
  int shmid = shmget(commonkey, mem_size, IPC_CREAT | IPC_EXCL | 0600);
  if (shmid < 0) {
    // If the shared memory already exists, attach to it
    if (errno == EEXIST) {
      shmid = shmget(commonkey, mem_size, 0);
      if (shmid < 0) {
        perror("shmget");
        return 1;
      }
      counter = (atomic_int *)shmat(shmid, NULL, 0);
      if (!counter) {
        perror("shmat");
        return 1;
      }
    } else {
      perror("shmget");
      return 1;
    }
  } else {
    // If the shared memory was created, initialize the counter
    should_remove = true;
    counter = (atomic_int *)shmat(shmid, NULL, 0);
    if (!counter) {
      perror("shmat");
      return 1;
    }
    atomic_store(counter, 0);
  }

  while (!finish_flag) {
    // Increment the counter
    int old_value = atomic_fetch_add(counter, 1);
    printf("Counter old value: %d\n", old_value);
    usleep(400000);
  }

  if (shmdt((void *)counter) < 0) {
    perror("shmdt");
    return 1;
  }

  // Mark to delete shmid
  if (should_remove) {
    rm_shmid(shmid);
  }

  return 0;
}