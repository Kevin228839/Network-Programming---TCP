#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <sys/signal.h>
using namespace std;

#define MAXLINE 25000
#define datasinkclient 5

int sockfd;
int sockfd2[datasinkclient];

void str_cli(int sockfd, int sockfd2[]) {
  char sendline[MAXLINE], recvline[MAXLINE];
  memset(sendline, 0, MAXLINE);
  memset(recvline, 0, MAXLINE);
  // send "reset command" to server
  memcpy(sendline, "/reset\n", 7);
  write(sockfd, sendline, strlen(sendline));
  read(sockfd, recvline, MAXLINE);
  fputs(recvline, stdout);
  // send dat to sink server
  memset(sendline, 'A', MAXLINE);
  while(1) {
    for(int i=0; i<datasinkclient; i++) {
      write(sockfd2[i], sendline, strlen(sendline));
    }
  }
}

void handler(int s) {
  // close data sink server connection
  for(int i=0; i<datasinkclient; i++) {
    close(sockfd2[i]);
  }
  // send "report command" to server
  char sendmessage[MAXLINE], recvmessage[MAXLINE];
  memset(sendmessage, 0, MAXLINE);
  memset(recvmessage, 0, MAXLINE);
  memcpy(sendmessage, "/report\n", 8);
  write(sockfd, sendmessage, strlen(sendmessage));
  read(sockfd, recvmessage, MAXLINE);
  fputs(recvmessage, stdout);
  exit(0);
}

int main(int argc, char* argv[]) {
  signal(SIGTERM, handler);

  if(argc != 3) {
    printf("usage: ./client <host> <port>\n");
    exit(1);
  }

  /* create connection to control server --start-- */
  struct sockaddr_in servaddr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(atoi(argv[2]));
  inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
  connect(sockfd, (sockaddr*) &servaddr, sizeof(servaddr));
  /* create connection to control server --end-- */

  /* create connection to sink data server --start-- */
  for(int i=0 ;i<datasinkclient; i++) {
    sockfd2[i] = socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2])+1);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    connect(sockfd2[i], (sockaddr*) &servaddr, sizeof(servaddr));
  }
  /* create connection to sink data server --end-- */


  str_cli(sockfd, sockfd2);

  exit(0);
}