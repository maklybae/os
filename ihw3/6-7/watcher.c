#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define RCVBUFSIZE 32

// Error handling function
void DieWithError(char *errorMessage);

volatile sig_atomic_t watcher_finish_flag = 0;
void handler_finish(int signum) { watcher_finish_flag = 1; }

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <Server IP> <Watcher Port>\n", argv[0]);
    exit(1);
  }

  char *servIP = argv[1];
  unsigned short watcherPort = atoi(argv[2]);

  // Create a reliable, stream socket using TCP
  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    DieWithError("socket() failed");

  // Construct the server address structure
  struct sockaddr_in servAddr;
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = inet_addr(servIP);
  servAddr.sin_port = htons(watcherPort);

  // Establish the connection to the server
  if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    DieWithError("connect() failed");

  char recvBuf[RCVBUFSIZE];
  int recvLen;
  // Receive and print logs from server
  while ((recvLen = recv(sock, recvBuf, RCVBUFSIZE - 1, 0)) > 0 &&
         !watcher_finish_flag) {
    recvBuf[recvLen] = '\0';
    printf("%s", recvBuf);
  }
  close(sock);
  return 0;
}
