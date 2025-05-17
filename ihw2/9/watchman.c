#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"

int sem_id = -1;
int msg_id = -1;
volatile sig_atomic_t finish_flag = 0;

void handler_finish(int signum) { finish_flag = 1; }

void CleanUp() {
  // Remove message queue
  if (msg_id != -1 && msgctl(msg_id, IPC_RMID, NULL) < 0) {
    perror("msgctl(IPC_RMID)");
  }

  // Remove semaphores
  if (sem_id != -1 && semctl(sem_id, 0, IPC_RMID) < 0) {
    perror("semctl IPC_RMID");
  }
}

int main() {
  // Signal handler
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Message queue
  key_t msg_key = ftok(MSG_KEY_FILENAME, 1);
  if (msg_key < 0) {
    perror("ftok for message queue");
    return 1;
  }

  msg_id = msgget(msg_key, IPC_CREAT | IPC_EXCL | 0600);
  if (msg_id < 0) {
    perror("msgget");
    return 1;
  }

  // Semaphores
  key_t sem_key = ftok(SEM_KEY_FILENAME, 1);
  if (sem_key == -1) {
    perror("ftok for semaphores");
    CleanUp();
    return 1;
  }

  // PAINTING_COUNT for paintings + 1 for gallery
  if ((sem_id = semget(sem_key, PAINTING_COUNT + 1,
                       IPC_CREAT | IPC_EXCL | 0600)) < 0) {
    perror("semget");
    CleanUp();
    return 1;
  }

  // Initialize semaphores
  unsigned short values[] = {MAX_VISITORS, MAX_VIEWERS, MAX_VIEWERS,
                             MAX_VIEWERS,  MAX_VIEWERS, MAX_VIEWERS};
  if (semctl(sem_id, 0, SETALL, values) < 0) {
    perror("semctl SETALL");
    CleanUp();
    return 1;
  }

  printf("Working day started...\n");

  // Process visitor completion messages until finish signal
  visitor_msg_t message;
  while (!finish_flag) {
    // Non-blocking message receive
    int result = msgrcv(msg_id, &message, sizeof(visitor_msg_t) - sizeof(long),
                        MSG_VISITOR_FINISHED, IPC_NOWAIT);

    if (result < 0) {
      if (errno == ENOMSG) {
        // No message available, just wait a bit
        usleep(100000); // 100ms
        continue;
      } else if (errno == EINTR) {
        // Interrupted by signal
        continue;
      } else {
        perror("msgrcv");
        CleanUp();
        return 1;
      }
    }

    // Process visitor completion message
    printf("Watchman: Visitor %d completed viewing all paintings\n",
           message.pid);

    // Signal gallery semaphore to allow another visitor
    struct sembuf leave_arg[1] = {{.sem_num = 0, .sem_op = 1, .sem_flg = 0}};
    if (semop(sem_id, leave_arg, 1) < 0) {
      if (errno != EINTR) {
        perror("semop (gallery release)");
        CleanUp();
        return 1;
      }
    }
  }

  printf("Working day finished...\n");
  CleanUp();
  return 0;
}
