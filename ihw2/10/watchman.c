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
#include <sys/types.h>
#include <time.h> // Add this header for clock_gettime and CLOCK_REALTIME
#include <unistd.h>

#include "common.h"

sem_t *gallery_sem = SEM_FAILED;
sem_t *painting_sems[PAINTING_COUNT] = {NULL};
mqd_t msg_queue = (mqd_t)-1;
volatile sig_atomic_t finish_flag = 0;

void handler_finish(int signum) { finish_flag = 1; }

// Create semaphore name for painting
void get_painting_sem_name(char *buffer, int painting_num) {
  snprintf(buffer, PAINTING_SEM_NAME_LEN, "%s%d", PAINTING_SEM_PREFIX,
           painting_num);
}

void CleanUp() {
  // Close and unlink message queue
  if (msg_queue != (mqd_t)-1) {
    mq_close(msg_queue);
    mq_unlink(GALLERY_MQ_NAME);
  }

  // Close and unlink semaphores
  if (gallery_sem != SEM_FAILED) {
    sem_close(gallery_sem);
    sem_unlink(GALLERY_SEM_NAME);
  }

  // Close and unlink painting semaphores
  for (int i = 0; i < PAINTING_COUNT; i++) {
    if (painting_sems[i] != NULL) {
      char sem_name[PAINTING_SEM_NAME_LEN];
      get_painting_sem_name(sem_name, i + 1);
      sem_close(painting_sems[i]);
      sem_unlink(sem_name);
    }
  }
}

int main() {
  // Signal handler
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Create message queue
  struct mq_attr queue_attr = {.mq_flags = 0,
                               // Limit 10 (i tried MAX_VISITORS=25)
                               .mq_maxmsg = 10,
                               .mq_msgsize = sizeof(visitor_msg_t),
                               .mq_curmsgs = 0};
  msg_queue = mq_open(GALLERY_MQ_NAME, O_CREAT | O_EXCL, 0600, &queue_attr);
  if (msg_queue == (mqd_t)-1) {
    perror("mq_open");
    CleanUp();
    return 1;
  }

  // Create gallery semaphore
  sem_unlink(GALLERY_SEM_NAME);
  gallery_sem =
      sem_open(GALLERY_SEM_NAME, O_CREAT | O_EXCL, 0600, MAX_VISITORS);
  if (gallery_sem == SEM_FAILED) {
    perror("sem_open (gallery)");
    CleanUp();
    return 1;
  }

  // Create painting semaphores
  for (int i = 0; i < PAINTING_COUNT; i++) {
    char sem_name[PAINTING_SEM_NAME_LEN];
    get_painting_sem_name(sem_name, i + 1);

    sem_unlink(sem_name);
    painting_sems[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0600, MAX_VIEWERS);
    if (painting_sems[i] == SEM_FAILED) {
      perror("sem_open (painting)");
      CleanUp();
      return 1;
    }
  }

  printf("Working day started...\n");

  // Process visitor completion messages until finish signal
  visitor_msg_t message;
  struct timespec timeout;

  while (!finish_flag) {
    // Get current time and add timeout
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += 1; // 1 second timeout

    // Non-blocking message receive
    ssize_t result = mq_timedreceive(msg_queue, (char *)&message,
                                     sizeof(message), NULL, &timeout);

    if (result < 0) {
      if (errno == ETIMEDOUT || errno == EAGAIN) {
        // No message available or timed out, just continue
        continue;
      } else if (errno == EINTR) {
        // Interrupted by signal
        continue;
      } else {
        perror("mq_timedreceive");
        CleanUp();
        return 1;
      }
    }

    // Process visitor completion message
    if (message.mtype == MSG_VISITOR_FINISHED) {
      printf("Watchman: Visitor %d completed viewing all paintings\n",
             message.pid);

      // Signal gallery semaphore to allow another visitor
      if (sem_post(gallery_sem) < 0) {
        perror("sem_post (gallery release)");
        CleanUp();
        return 1;
      }
    }
  }

  printf("Working day finished...\n");
  CleanUp();
  return 0;
}
