#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

volatile sig_atomic_t _Atomic finish_flag = 0;
void handler_finish(int signum) { finish_flag = 1; }

const char *SHMEM_FILENAME = "/counter";

void rm_shmid(int shmid) {
  if (shmctl(shmid, IPC_RMID, NULL) < 0) {
    perror("shmctl(IPC_RMID)");
    exit(1);
  }
}

int main(int argc, char const *argv[]) {
  bool should_unlink = false;

  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  volatile atomic_int *counter = NULL;

  size_t mem_size = getpagesize();
  int fd = shm_open(SHMEM_FILENAME, O_RDWR | O_CREAT | O_EXCL, 0600);
  if (fd < 0) {
    // If the shared memory already exists, attach to it
    if (errno == EEXIST) {
      fd = shm_open(SHMEM_FILENAME, O_RDWR, 0);
      if (fd < 0) {
        perror("shm_open");
        return 1;
      }

      counter = (atomic_int *)mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, fd, 0);
      if (!counter) {
        perror("mmap");
        return 1;
      }
    } else {
      perror("shm_open");
      return 1;
    }
  } else {
    // If the shared memory was created, initialize the counter
    should_unlink = true;

    if (ftruncate(fd, mem_size) < 0) {
      perror("ftruncate");
      return 1;
    }

    counter = (atomic_int *)mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fd, 0);
    if (!counter) {
      perror("mmap");
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

  if (munmap((void *)counter, mem_size) < 0) {
    perror("munmap");
    return 1;
  }

  if (should_unlink) {
    if (shm_unlink(SHMEM_FILENAME) < 0) {
      perror("shm_unlink");
      return 1;
    }
  }

  if (close(fd) < 0) {
    perror("close");
    return 1;
  }

  return 0;
}