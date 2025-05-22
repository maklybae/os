#include <arpa/inet.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define RCVBUFSIZE 32
#define MAX_PREPARE_TIME 10
#define MAX_TICKET_NUM 100

// Error handling function
void DieWithError(char *errorMessage);

volatile atomic_int _Atomic sock = 0;
void handler_finish(int signum) {
  printf("[CLIENT %d] interrupted\n", getpid());
  if (sock >= 0) {
    close(sock);
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  srand(getpid());

  struct sigaction act_finish = {.sa_handler = &handler_finish};
  if (sigaction(SIGINT, &act_finish, NULL) == -1)
    DieWithError("sigaction() failed");

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
    exit(1);
  }

  char *servIP = argv[1];
  unsigned short servPort = atoi(argv[2]);

  // Create a reliable, stream socket using TCP
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    DieWithError("socket() failed");

  // Construct the server address structure
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(servPort);

  // Establish the connection to the server
  if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    DieWithError("connect() failed");

  // Send ticket number as JSON
  int ticket_num = 1 + rand() % MAX_TICKET_NUM;
  char ticketMsg[64];
  snprintf(ticketMsg, sizeof(ticketMsg), "{\"ticket\": %d}", ticket_num);
  if (send(sock, ticketMsg, strlen(ticketMsg), 0) != (ssize_t)strlen(ticketMsg))
    DieWithError("send() failed");

  // Wait for server response (OK)
  char recvBuf[RCVBUFSIZE];
  int recvLen = recv(sock, recvBuf, RCVBUFSIZE - 1, 0);
  if (recvLen <= 0) {
    printf("[CLIENT %d] %ld server closed connection or send nothing\n",
           getpid(), time(NULL));
    close(sock);
    return 0;
  }
  recvBuf[recvLen] = '\0';
  printf("[CLIENT %d] %ld recv: %s\n", getpid(), time(NULL), recvBuf);
  close(sock);
  // Flag to indicate that the socket is closed
  sock = -1;

  // Simulate thinking (preparing answer)
  int prepare_time = 1 + rand() % MAX_PREPARE_TIME;
  sleep(prepare_time);

  // Connect again to send answer
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    DieWithError("socket() failed");
  if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    DieWithError("connect() failed");

  char answerMsg[128];
  snprintf(answerMsg, sizeof(answerMsg),
           "{\"ticket\": %d, \"answer\": \"answer%d\"}", ticket_num,
           ticket_num);
  if (send(sock, answerMsg, strlen(answerMsg), 0) != (ssize_t)strlen(answerMsg))
    DieWithError("send() failed");

  recvLen = recv(sock, recvBuf, RCVBUFSIZE - 1, 0);
  if (recvLen <= 0) {
    printf("[CLIENT %d] %ld server closed connection or send nothing\n",
           getpid(), time(NULL));
    close(sock);
    return 0;
  }
  recvBuf[recvLen] = '\0';
  printf("[CLIENT %d] %ld recv: %s\n", getpid(), time(NULL), recvBuf);
  printf("Grade: %s\n", recvBuf);
  close(sock);
  return 0;
}
