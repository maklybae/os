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

void HandleTCPClient(int clntSocket) {
  char buffer[RCVBUFSIZE];
  int recvMsgSize;

  // Receive message from client
  if ((recvMsgSize = recv(clntSocket, buffer, RCVBUFSIZE - 1, 0)) < 0)
    DieWithError("recv() failed");
  buffer[recvMsgSize] = '\0';
  printf("[SERVER] %ld recv: %s\n", time(NULL), buffer);

  // Check if message is ticket only or ticket+answer
  if (strstr(buffer, "\"answer\"") == NULL) {
    // Only ticket received
    // Send OK
    if (send(clntSocket, "OK", 2, 0) != 2) {
      printf("[SERVER] %ld send failed\n", time(NULL));
    }
  } else {
    // Ticket and answer received
    // Simulate checking answer (random sleep)
    int check_time = 1 + rand() % MAX_CHECK_TIME;
    sleep(check_time);
    if (server_finish_flag) {
      printf("[SERVER] %ld checking interrupted\n", time(NULL));
      close(clntSocket);
      return;
    }

    // Send random grade (2-5)
    int grade = MIN_GRADE + rand() % (MAX_GRADE - MIN_GRADE + 1);
    char gradeStr[4];
    snprintf(gradeStr, sizeof(gradeStr), "%d", grade);
    if (send(clntSocket, gradeStr, strlen(gradeStr), 0) !=
        (ssize_t)strlen(gradeStr)) {
      printf("[SERVER] %ld send failed\n", time(NULL));
    }
  }
  close(clntSocket);
}

int main(int argc, char *argv[]) {
  srand(getpid());

  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1)
    DieWithError("sigaction() failed");

  if (argc != 2) {
    fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
    exit(1);
  }

  unsigned short echoServPort = atoi(argv[1]);

  // Create socket for incoming connections
  int servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (servSock < 0)
    DieWithError("socket() failed");

  // Construct local address structure
  struct sockaddr_in echoServAddr;
  memset(&echoServAddr, 0, sizeof(echoServAddr));
  echoServAddr.sin_family = AF_INET;
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  echoServAddr.sin_port = htons(echoServPort);

  // Bind to the local address
  if (bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) <
      0)
    DieWithError("bind() failed");

  char server_ip[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &echoServAddr.sin_addr, server_ip,
                sizeof(server_ip)) == NULL)
    strcpy(server_ip, "unknown");
  printf("[SERVER] %ld Server started. IP: %s, Port: %d. Wait...\n", time(NULL),
         server_ip, echoServPort);

  // Mark the socket so it will listen for incoming connections
  if (listen(servSock, MAXPENDING) < 0)
    DieWithError("listen() failed");

  struct sockaddr_in echoClntAddr;
  while (!server_finish_flag) {
    unsigned int clntLen = sizeof(echoClntAddr);
    // Wait for a client to connect
    int clntSock = accept(servSock, (struct sockaddr *)&echoClntAddr, &clntLen);
    if (clntSock < 0)
      DieWithError("accept() failed");

    // clntSock is connected to a client!
    printf("[SERVER] %ld Handling client %s\n", time(NULL),
           inet_ntoa(echoClntAddr.sin_addr));
    HandleTCPClient(clntSock);
  }

  // Close the socket
  if (close(servSock) < 0)
    DieWithError("close() failed");
  printf("Server closed.\n");
  return 0;
}
