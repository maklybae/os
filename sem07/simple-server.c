#include <sys/shm.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

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
	shmid = shmget(commonkey, mem_size, IPC_CREAT | IPC_EXCL | 0600);
	if (shmid < 0) {
		perror("shmget");
		goto err;
	}

	shmem = shmat(shmid, NULL, 0);
	if (!shmem) {
		perror("shmmem");
		goto err;
	}

	printf("Created shared mem with key=%d shmid=%d size=%d addr=%p\n", 
		   commonkey, shmid, mem_size, shmem);
	shmctl(shmid, IPC_RMID, NULL);
	*shmem = 1;

	// Wait until mem is changed
	while (*shmem) sched_yield();
	printf("Shutting down server\n");

	if (shmdt(shmem) < 0) {
		perror("shmdt");
		goto err;
	}

out:
	if (shmid >= 0) {
		if (shmctl(shmid, IPC_RMID, NULL) < 0) {
			perror("shmctl(IPC_RMID)");
		}

		// Let's see if it was actually removed
		struct shmid_ds buf = {};
		if (shmctl(shmid, IPC_STAT, &buf) == 0) {
			printf("Shared memory with shmid=%d still exists\n", shmid);
		} else if (errno == EIDRM || errno == EINVAL) {
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
