#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <initial file path>\n", argv[0]);
    return 1;
  }

  char prev_path[256], path[256];
  snprintf(prev_path, sizeof(prev_path), "%s", argv[1]);
  for (int depth = 0;; ++depth) {
    snprintf(path, sizeof(path), "%d_%s", depth, argv[1]);
    if (symlink(prev_path, path) < 0) {
      perror("symlink");
    }
    if (open(path, O_RDONLY) < 0) {
      if (errno == ELOOP) {
        printf("Maximum depth reached: %d\n", depth);
        break;
      } else {
        perror("open");
        break;
      }
    }
    snprintf(prev_path, sizeof(prev_path), "%s", path);
  }

  return 0;
}

// При тестовом запуске получилось создать 32 символьных ссылки, после чего
// возникла ошибка ELOOP.