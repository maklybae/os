#include <sys/_types/_ssize_t.h>

typedef struct {
  int data;
  ssize_t next;
} elem_t;

typedef struct {
  ssize_t head;
  elem_t elements[];
} linked_list_t;
