#include <sys/wait.h>
#include <unistd.h>

int main() {
  int fd[2];
  pipe(fd);

  pid_t cpid = fork();
  if (cpid == 0) {
    dup2(fd[0], 0);
    close(fd[0]);
    close(fd[1]);
    execl("/usr/bin/grep", "grep", "root", NULL);
  } else {
    dup2(fd[1], 1);
    close(fd[0]);
    close(fd[1]);
    execl("/bin/cat", "cat", "/etc/passwd", NULL);
    wait(NULL); // Wait for the child process to finish
  }

  return 0;
}