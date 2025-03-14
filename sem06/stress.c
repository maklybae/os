#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
    return 1;
  }

  pid_t pid = atoi(argv[1]);

  int counter = 0;
  while (1) {
    if (++counter == 100) {
      kill(pid, SIGUSR1);
      counter = 0;
    }
  }

  return 0;
}