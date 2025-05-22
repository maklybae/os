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
#define MAX_MONITOR_CLIENTS 10

// Error handling function
void DieWithError(char *errorMessage);

volatile sig_atomic_t server_finish_flag = 0;
void handler_finish(int signum) { server_finish_flag = 1; }

int monitor_clients[MAX_MONITOR_CLIENTS];
int monitor_clients_count = 0;

// Function to send a log message to all watcher clients
void send_log_to_watchers(const char *msg) {
  for (int i = 0; i < monitor_clients_count; ++i) {
    // Important to use MSG_NOSIGNAL to avoid block on sending to closed socket
    if (send(monitor_clients[i], msg, strlen(msg), MSG_NOSIGNAL) <= 0) {
      if (close(monitor_clients[i]) < 0)
        DieWithError("close() failed");
      for (int j = i; j < monitor_clients_count - 1; ++j)
        monitor_clients[j] = monitor_clients[j + 1];
      monitor_clients_count--;
      i--; // чтобы не пропустить следующий элемент
    }
  }
}

// Handle a single student client, log all events to watchers
void HandleTCPClient(int clntSocket) {
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
  send_log_to_watchers(logmsg);
  // Check if message is ticket only or ticket+answer
  if (strstr(buffer, "\"answer\"") == NULL) {
    // Only ticket received
    // Send OK
    snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send: OK\n", time(NULL));
    send_log_to_watchers(logmsg);
    if (send(clntSocket, "OK", 2, MSG_NOSIGNAL) != 2) {
      snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send failed\n",
               time(NULL));
      printf("%s", logmsg);
      send_log_to_watchers(logmsg);
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
      send_log_to_watchers(logmsg);
      close(clntSocket);
      return;
    }
    // Send random grade (2-5)
    int grade = MIN_GRADE + rand() % (MAX_GRADE - MIN_GRADE + 1);
    char gradeStr[4];
    snprintf(gradeStr, sizeof(gradeStr), "%d", grade);
    snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send: %s\n", time(NULL),
             gradeStr);
    send_log_to_watchers(logmsg);
    if (send(clntSocket, gradeStr, strlen(gradeStr), MSG_NOSIGNAL) !=
        (ssize_t)strlen(gradeStr)) {
      snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld send failed\n",
               time(NULL));
      printf("%s", logmsg);
      send_log_to_watchers(logmsg);
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
  if (listen(watcherSock, MAX_MONITOR_CLIENTS) < 0)
    DieWithError("watcher listen() failed");

  printf("[SERVER] %ld Server started. Student port: %d, Watcher port: %d. "
         "Wait...\n",
         time(NULL), echoServPort, watcherPort);

  struct sockaddr_in echoClntAddr;
  struct sockaddr_in watcherClntAddr;
  fd_set readfds;

  while (!server_finish_flag) {
    int maxfd = servSock > watcherSock ? servSock : watcherSock;
    FD_ZERO(&readfds);
    FD_SET(servSock, &readfds);
    FD_SET(watcherSock, &readfds);
    for (int i = 0; i < monitor_clients_count; ++i) {
      FD_SET(monitor_clients[i], &readfds);
      if (monitor_clients[i] > maxfd)
        maxfd = monitor_clients[i];
    }
    int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (server_finish_flag)
      break;
    if (activity < 0 && !server_finish_flag)
      DieWithError("select() failed");

    // Watcher client connected
    if (FD_ISSET(watcherSock, &readfds)) {
      unsigned int watcherClntLen = sizeof(watcherClntAddr);
      int new_monitor_sock = accept(
          watcherSock, (struct sockaddr *)&watcherClntAddr, &watcherClntLen);
      if (new_monitor_sock < 0)
        DieWithError("accept() failed for watcher");
      if (monitor_clients_count < MAX_MONITOR_CLIENTS) {
        monitor_clients[monitor_clients_count++] = new_monitor_sock;
        char logmsg[256];
        snprintf(logmsg, sizeof(logmsg),
                 "[SERVER] %ld Watcher connected from %s\n", time(NULL),
                 inet_ntoa(watcherClntAddr.sin_addr));
        printf("%s", logmsg);
        send_log_to_watchers(logmsg);
      } else {
        close(new_monitor_sock);
      }
    }

    // Student client connected
    if (FD_ISSET(servSock, &readfds)) {
      unsigned int clntLen = sizeof(echoClntAddr);
      int clntSock =
          accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen);
      if (clntSock < 0)
        DieWithError("accept() failed");
      char logmsg[256];
      snprintf(logmsg, sizeof(logmsg), "[SERVER] %ld Handling client %s\n",
               time(NULL), inet_ntoa(echoClntAddr.sin_addr));
      printf("%s", logmsg);
      send_log_to_watchers(logmsg);
      HandleTCPClient(clntSock);
    }
  }
  if (close(servSock) < 0)
    DieWithError("close() failed");
  if (close(watcherSock) < 0)
    DieWithError("close() failed");
  for (int i = 0; i < monitor_clients_count; ++i)
    if (close(monitor_clients[i]))
      DieWithError("close() failed");
  printf("Server closed.\n");
  return 0;
}
