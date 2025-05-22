#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define RCVBUFSIZE 256
#define MAXPENDING 20
#define MAX_CHECK_TIME 5
#define MIN_GRADE 2
#define MAX_GRADE 5

// Error handling function
void DieWithError(char *errorMessage);

volatile sig_atomic_t server_finish_flag = 0;
void handler_finish(int signum) { server_finish_flag = 1; }

// Function to send a log message to the watcher client
void send_log_to_watcher(int watcher_sock, const char *msg) {
  if (watcher_sock > 0) {
    send(watcher_sock, msg, strlen(msg), 0);
  }
}

// Handle a single student client, log all events to watcher
void HandleTCPClient(int clntSocket, int watcherClientSock) {
  char buffer[RCVBUFSIZE];
  int recvMsgSize;
  // Receive message from client
  if ((recvMsgSize = recv(clntSocket, buffer, RCVBUFSIZE - 1, 0)) < 0)
    DieWithError("recv() failed");
  buffer[recvMsgSize] = '\0';
  char logmsg[256];
  snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld recv: %s\n", time(NULL),
           buffer);
  printf("%s", logmsg);
  send_log_to_watcher(watcherClientSock, logmsg);
  // Check if message is ticket only or ticket+answer
  if (strstr(buffer, "\"answer\"") == NULL) {
    // Only ticket received
    // Send OK
    snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send: OK\n", time(NULL));
    send_log_to_watcher(watcherClientSock, logmsg);
    if (send(clntSocket, "OK", 2, 0) != 2) {
      snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send failed\n",
               time(NULL));
      printf("%s", logmsg);
      send_log_to_watcher(watcherClientSock, logmsg);
    }
  } else {
    // Ticket and answer received
    // Simulate checking answer (random sleep)
    int check_time = 1 + rand() % MAX_CHECK_TIME;
    sleep(check_time);
    if (server_finish_flag) {
      snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld checking interrupted\n",
               time(NULL));
      printf("%s", logmsg);
      send_log_to_watcher(watcherClientSock, logmsg);
      close(clntSocket);
      return;
    }
    // Send random grade (2-5)
    int grade = MIN_GRADE + rand() % (MAX_GRADE - MIN_GRADE + 1);
    char gradeStr[4];
    snprintf(gradeStr, sizeof(gradeStr), "%d", grade);
    snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send: %s\n", time(NULL),
             gradeStr);
    send_log_to_watcher(watcherClientSock, logmsg);
    if (send(clntSocket, gradeStr, strlen(gradeStr), 0) !=
        (ssize_t)strlen(gradeStr)) {
      snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send failed\n",
               time(NULL));
      printf("%s", logmsg);
      send_log_to_watcher(watcherClientSock, logmsg);
    }
  }
  close(clntSocket);
}

int main(int argc, char *argv[]) {
  srand(getpid());

  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1)
    DieWithError("sigaction() failed");

  if (argc != 3) {
    fprintf(stderr, "Usage:  %s <Server Port> <Watcher Port>\n", argv[0]);
    exit(1);
  }

  unsigned short echoServPort = atoi(argv[1]);
  unsigned short watcherPort = atoi(argv[2]);

  // --- Socket for student clients ---
  int servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (servSock < 0)
    DieWithError("socket() failed");
  struct sockaddr_in echoServAddr;
  memset(&echoServAddr, 0, sizeof(echoServAddr));
  echoServAddr.sin_family = AF_INET;
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  echoServAddr.sin_port = htons(echoServPort);
  if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) <
      0)
    DieWithError("bind() failed");
  if (listen(servSock, MAXPENDING) < 0)
    DieWithError("listen() failed");

  // --- Socket for watcher ---
  int watcherSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (watcherSock < 0)
    DieWithError("watcher socket() failed");
  struct sockaddr_in watcherAddr;
  memset(&watcherAddr, 0, sizeof(watcherAddr));
  watcherAddr.sin_family = AF_INET;
  watcherAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  watcherAddr.sin_port = htons(watcherPort);
  if (bind(watcherSock, (struct sockaddr *)&watcherAddr, sizeof(watcherAddr)) <
      0)
    DieWithError("watcher bind() failed");
  if (listen(watcherSock, 1) < 0)
    DieWithError("watcher listen() failed");

  printf("[SERVER] %ld Server started. Student port: %d, Watcher port: %d. "
         "Wait...\n",
         time(NULL), echoServPort, watcherPort);

  // Wait for watcher connection
  int watcherClientSock = -1;
  struct sockaddr_in watcherClntAddr;
  unsigned int watcherClntLen = sizeof(watcherClntAddr);
  printf("[SERVER] %ld Waiting for watcher...\n", time(NULL));
  watcherClientSock =
      accept(watcherSock, (struct sockaddr *)&watcherClntAddr, &watcherClntLen);
  if (watcherClientSock < 0)
    DieWithError("accept() failed for watcher");
  printf("[SERVER] %ld Watcher connected from %s\n", time(NULL),
         inet_ntoa(watcherClntAddr.sin_addr));

  struct sockaddr_in echoClntAddr;
  while (!server_finish_flag) {
    unsigned int clntLen = sizeof(echoClntAddr);
    int clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen);
    if (clntSock < 0)
      DieWithError("accept() failed");
    char logmsg[256];
    snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld Handling client %s\n",
             time(NULL), inet_ntoa(echoClntAddr.sin_addr));
    printf("%s", logmsg);
    send_log_to_watcher(watcherClientSock, logmsg);
    HandleTCPClient(clntSock, watcherClientSock);
  }
  if (close(servSock) < 0)
    DieWithError("close() failed");
  if (close(watcherSock) < 0)
    DieWithError("close() failed");
  if (watcherClientSock > 0)
    close(watcherClientSock);
  printf("Server closed.\n");
  return 0;
}
