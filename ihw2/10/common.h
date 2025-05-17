#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>

#define GALLERY_SEM_NAME "/gallery-semaphore"
#define PAINTING_SEM_PREFIX "/painting-semaphore-"
#define PAINTING_SEM_NAME_LEN 32
#define MAX_VISITORS 25
#define PAINTING_COUNT 5
#define MAX_VIEWERS 5
#define MAX_WAITING_TIME 2
#define MIN_WAITING_TIME 1

#define GALLERY_MQ_NAME "/gallery-message-queue"

#define MSG_VISITOR_FINISHED 1

// Message structure for communication
typedef struct {
  int mtype; // Message type
  pid_t pid; // Visitor PID
} visitor_msg_t;

#endif // COMMON_H
