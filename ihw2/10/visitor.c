#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
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

sem_t *gallery_sem = SEM_FAILED;
sem_t *painting_sems[PAINTING_COUNT] = {NULL};
mqd_t msg_queue = (mqd_t)-1;

volatile sig_atomic_t visitor_finish_flag = 0;
void handler_finish(int signum) { visitor_finish_flag = 1; }

void CleanUp() {
  // Close all resources
  if (gallery_sem != SEM_FAILED) {
    sem_close(gallery_sem);
  }

  for (int i = 0; i < PAINTING_COUNT; i++) {
    if (painting_sems[i] != NULL) {
      sem_close(painting_sems[i]);
    }
  }

  if (msg_queue != (mqd_t)-1) {
    mq_close(msg_queue);
  }
}

int main() {
  // Signal handler
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Open message queue
  msg_queue = mq_open(GALLERY_MQ_NAME, O_WRONLY);
  if (msg_queue == (mqd_t)-1) {
    perror("mq_open");
    return 1;
  }

  // Open gallery semaphore
  gallery_sem = sem_open(GALLERY_SEM_NAME, 0);
  if (gallery_sem == SEM_FAILED) {
    perror("sem_open (gallery)");
    mq_close(msg_queue);
    return 1;
  }

  // Open painting semaphores
  for (int i = 0; i < PAINTING_COUNT; i++) {
    char sem_name[PAINTING_SEM_NAME_LEN];
    snprintf(sem_name, PAINTING_SEM_NAME_LEN, "%s%d", PAINTING_SEM_PREFIX,
             i + 1);

    painting_sems[i] = sem_open(sem_name, 0);
    if (painting_sems[i] == SEM_FAILED) {
      perror("sem_open (painting)");

      // Close previously opened semaphores
      sem_close(gallery_sem);
      for (int j = 0; j < i; j++) {
        sem_close(painting_sems[j]);
      }
      mq_close(msg_queue);
      return 1;
    }
  }

  // Init random number generator
  srand(getpid());

  // Wait for gallery semaphore
  if (sem_wait(gallery_sem) < 0) {
    if (errno == EINTR) {
      // Interrupted by signal
      CleanUp();
      return 0;
    }
    perror("sem_wait (gallery enter)");
    CleanUp();
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
    if (sem_wait(painting_sems[next_painting]) < 0) {
      if (errno == EINTR) {
        // Interrupted by signal
        if (visitor_finish_flag) {
          break;
        }
        continue;
      }
      perror("sem_wait (painting)");
      CleanUp();
      return 1;
    }

    printf("Visitor %d is viewing painting %d\n", getpid(), next_painting + 1);
    sleep(rand() % (MAX_WAITING_TIME - MIN_WAITING_TIME + 1) +
          MIN_WAITING_TIME);
    visited |= 1 << next_painting;

    // Signal painting semaphore
    if (sem_post(painting_sems[next_painting]) < 0) {
      perror("sem_post (painting)");
      CleanUp();
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

    if (mq_send(msg_queue, (const char *)&message, sizeof(message), 0) < 0) {
      perror("mq_send");
      CleanUp();
      return 1;
    }

    printf("Visitor %d has seen all paintings and left the gallery\n",
           getpid());
  } else {
    // If interrupted before seeing all paintings, still need to free the
    // semaphore
    if (sem_post(gallery_sem) < 0) {
      perror("sem_post (gallery leave)");
    }
    printf("Visitor %d left the gallery early\n", getpid());
  }

  CleanUp();
  return 0;
}
