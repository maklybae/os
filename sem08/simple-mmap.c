#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>       
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    int fd = open("private_file", O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    size_t mem_size = getpagesize();

    if (ftruncate(fd, mem_size) < 0) {
        perror("ftruncate");
        return 1;
    }

    char *shmem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
                       /*MAP_PRIVATE*/ MAP_SHARED, fd, 0);
    if (!shmem) {
        perror("mmap");
        return 1;
    }
    
    const char msg[] = "ABCDEF____1010110";
    memcpy(shmem, msg, sizeof(msg));

    if (munmap(shmem, mem_size) < 0) {
        perror("munmap");
        return 1;
    }

    close(fd);

    return 0;
}
