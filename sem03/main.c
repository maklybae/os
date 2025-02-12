#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/_types/_pid_t.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <argument>\n", argv[0]);
    return 1;
  }

  uint64_t n;
  if (sscanf(argv[1], "%lld", &n) != 1) {
    fprintf(stderr, "Invalid argument\n");
    return 1;
  }

  pid_t cpid = fork();
  if (cpid < 0) {
    perror("fork");
    return 1;
  }

  if (cpid == 0) {
    // factorial in child
    if (n == 0) {
      printf("%lld! is 1", n);
      return 0;
    }

    uint64_t result = 1;
    for (uint64_t i = 2; i <= n; i++) {
      if (result > ULLONG_MAX / i) {
        fprintf(stderr, "Overflow while calculating %lld!\n", n);
        return 1;
      }
      result *= i;
    }
    printf("%lld! is %lld\n", n, result);
  } else {
    // fibonacci in parent
    if (n == 0) {
      printf("%lld-th Fibonacci number is 0", n);
      return 0;
    }

    uint64_t a = 0;
    uint64_t b = 1;
    for (uint64_t i = 2; i <= n; i++) {
      if (a > ULLONG_MAX - b) {
        fprintf(stderr, "Overflow while calculating %lld-th Fibonacci number\n",
                n);
        return 1;
      }
      uint64_t c = a + b;
      a = b;
      b = c;
    }
    printf("%lld-th Fibonacci number is %lld\n", n, b);

    wait(NULL);
    pid_t execl_cpid = fork();
    if (execl_cpid < 0) {
      perror("fork");
      return 1;
    }

    if (execl_cpid == 0) {
      // execl in new process with fork-exec
      printf("Executing ls -lah\n");
      execl("/bin/ls", "ls", "-lah", NULL);
      perror("execl");
      return 1;
    }
  }

  return 0;
}