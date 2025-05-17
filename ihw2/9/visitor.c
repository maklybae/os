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

enum PaintingFlags {
  PAINTING_1 = 1 << 0, // 00001
  PAINTING_2 = 1 << 1, // 00010
  PAINTING_3 = 1 << 2, // 00100
  PAINTING_4 = 1 << 3, // 01000
  PAINTING_5 = 1 << 4, // 10000
  ALL_PAINTINGS = 0x1F // 11111
};

int sem_id = -1;
int msg_id = -1;

volatile sig_atomic_t visitor_finish_flag = 0;
void handler_finish(int signum) { visitor_finish_flag = 1; }

int main() {
  // Signal handler
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Access message queue
  key_t msg_key = ftok(MSG_KEY_FILENAME, 1);
  if (msg_key < 0) {
    perror("ftok for message queue");
    return 1;
  }

  msg_id = msgget(msg_key, 0);
  if (msg_id < 0) {
    perror("msgget");
    return 1;
  }

  // Access semaphores
  key_t sem_key = ftok(SEM_KEY_FILENAME, 1);
  if (sem_key == -1) {
    perror("ftok for semaphores");
    return 1;
  }

  // PAINTING_COUNT for paintings + 1 for gallery
  if ((sem_id = semget(sem_key, PAINTING_COUNT + 1, 0)) < 0) {
    perror("semget");
    return 1;
  }

  // Init random number generator
  srand(getpid());

  // Wait for gallery semaphore
  struct sembuf enter_arg[1] = {{.sem_num = 0, .sem_op = -1, .sem_flg = 0}};
  if (semop(sem_id, enter_arg, 1) < 0) {
    if (errno == EINTR) {
      // Interrupted by signal
      return 0;
    }
    perror("semop (gallery enter)");
    return 1;
  }

  printf("Visitor %d entered the gallery\n", getpid());

  uint8_t visited = 0;
  while (visited != ALL_PAINTINGS && !visitor_finish_flag) {
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
        if (visitor_finish_flag) {
          break;
        }
        continue;
      }
      perror("semop (painting wait)");
      return 1;
    }

    printf("Visitor %d is viewing painting %d\n", getpid(), next_painting + 1);
    sleep(rand() % (MAX_WAITING_TIME - MIN_WAITING_TIME + 1) +
          MIN_WAITING_TIME);
    visited |= 1 << next_painting;

    // Signal painting semaphore
    struct sembuf paint_post_arg[1] = {
        {.sem_num = next_painting + 1, .sem_op = 1, .sem_flg = 0}};
    if (semop(sem_id, paint_post_arg, 1) < 0) {
      if (errno == EINTR) {
        if (visitor_finish_flag) {
          break;
        }
        continue;
      }
      perror("semop (painting release)");
      return 1;
    }

    printf("Visitor %d finished viewing painting %d\n", getpid(),
           next_painting + 1);
  }

  if (visited == ALL_PAINTINGS) {
    // Send message to watchman that visitor is leaving
    visitor_msg_t message;
    message.mtype = MSG_VISITOR_FINISHED;
    message.pid = getpid();

    if (msgsnd(msg_id, &message, sizeof(visitor_msg_t) - sizeof(long), 0) < 0) {
      perror("msgsnd");
      return 1;
    }

    printf("Visitor %d has seen all paintings and left the gallery\n",
           getpid());
  } else {
    // If interrupted before seeing all paintings, still need to free the
    // semaphore
    struct sembuf leave_arg[1] = {{.sem_num = 0, .sem_op = 1, .sem_flg = 0}};
    if (semop(sem_id, leave_arg, 1) < 0 && errno != EINTR) {
      perror("semop (gallery leave)");
    }
    printf("Visitor %d left the gallery early\n", getpid());
  }

  return 0;
}
