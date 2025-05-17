#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>

#define PAINTING_COUNT 5
#define MAX_VISITORS 25
#define MAX_VIEWERS 5

#define MIN_WAITING_TIME 1
#define MAX_WAITING_TIME 2

#define SEM_KEY_FILENAME "semkey"
#define MSG_KEY_FILENAME "msgkey"

// Message types
#define MSG_VISITOR_FINISHED 1

// Message structure for communication
typedef struct {
  long mtype; // Message type
  pid_t pid;  // Visitor PID
} visitor_msg_t;

#endif // COMMON_H
