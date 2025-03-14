#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

const char *log_files[] = {"logs-1", "logs-2"};

volatile sig_atomic_t _Atomic finish_flag = 0;
volatile sig_atomic_t _Atomic switch_flag = 0;

// С семинара:
// В идеальном случае обработчик должен не делать ничего
// (SIG_IGN) или поставить один флаг типа volatile sig_atomic_t.
// Основная логика обработки должна быть перенесена в обычный поток
// выполнения. Например, написать цикл, который регулярно будет проверять
// значение флага, изменяемого из обработчика сигнала.
// От себя:
// Такой подход позволяет нам избежать race condition, так как
// файловые дескрипторы не относятся к общим данным, разделяемым между
// обработчиком и основным потоком выполнения.
void handler_finish(int signum) { finish_flag = 1; }
void handler_switch(int signum) { switch_flag = 1; }

int main() {
  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  struct sigaction act_switch = {.sa_handler = &handler_switch};
  if (sigaction(SIGUSR1, &act_switch, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  int current_to = 0;
  int fd_to = open(log_files[current_to], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd_to < 0) {
    perror("open");
    return 1;
  }

  while (!finish_flag) {
    if (switch_flag) {
      current_to = (current_to + 1) % 2;
      if (close(fd_to) < 0) {
        perror("close");
        return 1;
      }

      fd_to = open(log_files[current_to], O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd_to < 0) {
        perror("open");
        return 1;
      }

      switch_flag = 0;
    }

    time_t rawtime;
    time(&rawtime);

    char *time_str = ctime(&rawtime);
    if (time_str == NULL) {
      perror("ctime");
      return 1;
    }

    if (write(fd_to, time_str, strlen(time_str)) < 0) {
      perror("write");
      return 1;
    }

    sleep(1);
  }

  if (close(fd_to) < 0) {
    perror("close");
    return 1;
  }

  return 0;
}
