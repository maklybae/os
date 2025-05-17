#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_VISITORS 25
#define PAINTING_COUNT 5
#define MAX_VIEWERS 5
#define MAX_WAITING_TIME 5
#define MIN_WAITING_TIME 1

enum PaintingFlags {
  PAINTING_1 = 1 << 0, // 00001
  PAINTING_2 = 1 << 1, // 00010
  PAINTING_3 = 1 << 2, // 00100
  PAINTING_4 = 1 << 3, // 01000
  PAINTING_5 = 1 << 4, // 10000
  ALL_PAINTINGS = 0x1F // 11111
};

volatile sig_atomic_t _Atomic finish_flag = 0;
void handler_finish(int signum) { finish_flag = 1; }

sem_t gallery_sem;
sem_t painting_sems[PAINTING_COUNT];

void CleanUp(int paintings) {
  // Close and unlink semaphores
  if (sem_destroy(&gallery_sem) == -1) {
    perror("sem_destroy");
  }
  for (int i = 0; i < PAINTING_COUNT; ++i) {
    if (sem_destroy(&painting_sems[i]) == -1) {
      perror("sem_destroy");
    }
  }
}

int main(int argc, char *argv[]) {
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Read number of visitors from command line
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <visitors>\n", argv[0]);
    return 1;
  }

  int visitors;
  if (sscanf(argv[1], "%d", &visitors) != 1) {
    fprintf(stderr, "Invalid argument\n");
    return 1;
  }
  if (visitors < 1) {
    fprintf(stderr, "Number of visitors must be at least 1\n");
    return 1;
  }

  printf("Working day started...\n");

  // Inint gallery semaphore
  if (sem_init(&gallery_sem, true, MAX_VISITORS) == -1) {
    perror("sem_init");
    return 1;
  };

  // Initialize painting semaphores (stored in array)
  for (int i = 0; i < PAINTING_COUNT; ++i) {
    if (sem_init(&painting_sems[i], true, MAX_VIEWERS) == -1) {
      perror("sem_init");

      // Clean up only successfully created semaphores
      CleanUp(i);
      return 1;
    }
  }

  // Fork visitor processes
  for (int i = 0; i < visitors; ++i) {
    pid_t cpid = fork();
    if (cpid < 0) {
      perror("fork");
      return 1;
    }

    if (cpid == 0) {
      // Visitor process

      // Init random number generator (use pid as seed to avoid same seed for
      // all visitors)
      srand(getpid());

      // Wait for gallery semaphore
      if (sem_wait(&gallery_sem) == -1) {
        perror("sem_wait");
        return 1;
      }
      printf("Visitor %d entered the gallery\n", getpid());

      uint8_t visited = 0;
      while (visited != ALL_PAINTINGS && !finish_flag) {
        // Randomly select a painting to view
        // Ensure that the selected painting has not been viewed yet
        uint8_t next_painting;
        while (1 << (next_painting = rand() % PAINTING_COUNT) & visited) {
        }

        // Wait for painting semaphore
        if (sem_wait(&painting_sems[next_painting]) == -1) {
          perror("sem_wait");
          return 1;
        }

        printf("Visitor %d is viewing painting %d\n", getpid(),
               next_painting + 1);
        sleep(rand() % (MAX_WAITING_TIME - MIN_WAITING_TIME + 1) +
              MIN_WAITING_TIME);
        visited |= 1 << next_painting;

        // Signal painting semaphore
        if (sem_post(&painting_sems[next_painting]) == -1) {
          perror("sem_post");
          return 1;
        }
        printf("Visitor %d finished viewing painting %d\n", getpid(),
               next_painting + 1);
      }

      // Signal gallery semaphore
      if (sem_post(&gallery_sem) == -1) {
        perror("sem_post");
        return 1;
      }
      printf("Visitor %d left the gallery\n", getpid());

      // Exit visitor process
      return 0;
    }
  }

  // Wait for all visitors to finish
  for (int i = 0; i < visitors; ++i) {
    pid_t wpid;

    // If SIGINT interrupted wait, continue waiting
    do {
      wpid = wait(NULL);
    } while (wpid == -1 && errno == EINTR);

    if (wpid == -1) {
      perror("wait");
      break;
    }
  }

  CleanUp(PAINTING_COUNT);
  printf("Working day finished...\n");
  return 0;
}
