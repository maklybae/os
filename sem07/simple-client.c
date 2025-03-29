#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char const *argv[]) {
	int return_code = 0;
	int shmid;
	key_t commonkey;
	int *shmem;
	int mem_size;

	shmid = -1;

	commonkey = ftok("./keyfile", 1);
	if (commonkey < 0) {
		perror("commonkey");
		goto err;
	}

	mem_size = getpagesize();
	shmid = shmget(commonkey, mem_size, 0);
    // Note: no IPC_CREAT
	if (shmid < 0) {
		perror("shmget");
		goto err;
	}

	shmem = shmat(shmid, NULL, 0);
	if (!shmem) {
		perror("shmmem");
		goto err;
	}

	printf("Opened shared mem with key=%d shmid=%d size=%d addr=%p\n", 
		   commonkey, shmid, mem_size, shmem);
	
    // Shut down the server
    *shmem = 0;

	if (shmdt(shmem) < 0) {
		perror("shmdt");
		goto err;
	}

out:
	if (shmid >= 0) {
		if (shmctl(shmid, IPC_RMID, NULL) < 0) {
			perror("shmctl(IPC_RMID)");
		}

		struct shmid_ds buf;
		if (shmctl(shmid, IPC_STAT, &buf) == 0) {
			printf("Shared memory with shmid=%d still exists\n", shmid);
		} else if (errno == EIDRM) {
			printf("Shared memory with shmid=%d removed\n", shmid);
		} else {
			perror("shmctl(IPC_STAT)");
		}
	}

	return return_code;

err:
	return_code = 1;
	goto out;
}