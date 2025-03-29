#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>       
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <time.h>

#include "common.h"

volatile sig_atomic_t running = 1;
void sigint(int) {
    running = 0;
}

int main(int argc, char const *argv[])
{
    struct sigaction sact = {.sa_handler = &sigint};
    if (sigaction(SIGINT, &sact, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    int fd = open(SHMEM_FILENAME, O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    size_t mem_size = getpagesize();

    if (ftruncate(fd, mem_size) < 0) {
        perror("ftruncate");
        return 1;
    }

    size_t msg_capacity = mem_size - sizeof(shared_mem_t);
    shared_mem_t *shmem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (!shmem) {
        perror("mmap");
        return 1;
    }
    
    while (running) {
        shmem->msg_len = snprintf(shmem->msg, msg_capacity,
                                  "Current time: %ld", time(NULL));
        shmem->rdy = 1;

        printf("Sending next msg...\n");
        sleep(1);

        while (running && shmem->rdy) sched_yield();
    }

    if (munmap(shmem, mem_size) < 0) {
        perror("munmap");
        return 1;
    }

    if (close(fd) < 0) {
        perror("close");
        return 1;
    }

    return 0;
}
