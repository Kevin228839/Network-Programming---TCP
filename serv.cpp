#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <sys/time.h>
#define LISTENQ 1024
#define MAXLINE 30000
#define itoalen 50

void timestring(char* timearray, char* itoa) {
  struct timeval _t1;
  gettimeofday(&_t1, NULL);

  memset(timearray, 0, 50);
  memset(itoa, 0, itoalen);
  sprintf(itoa, "%lf", _t1.tv_sec+0.000001*_t1.tv_usec);
  strcat(timearray, itoa);
}

void timediff(char* timediffarray, char* itoa, timeval last) {
  struct timeval now;
  gettimeofday(&now, NULL);

  memset(timediffarray, 0, 50);
  memset(itoa, 0, itoalen);
  sprintf(itoa, "%lf", (now.tv_sec+0.000001*now.tv_usec)-(last.tv_sec+0.000001*last.tv_usec));
  strcat(timediffarray, itoa);
}

void handler(int s) {
  // catch sigpipe
}

int main(int argc, char* argv[]) {
  signal(SIGPIPE, handler);
  // check command's arguments
  if(argc != 2) {
    printf("usage: ./serv <port>\n");
    exit(1);
  }

  int	i, maxi, maxfd, listenfd, connfd, sockfd, j;
  int listenfd2;
	int	nready, client[FD_SETSIZE];
	ssize_t	n;
	fd_set rset, allset;
	char buf[MAXLINE];
	socklen_t	clilen;
	struct sockaddr_in	cliaddr, servaddr;
  char ipstr[INET_ADDRSTRLEN];
  char serverreply[MAXLINE];
  char timearray[50];
  char timediffarray[50];
  char itoa[50];
  int count = 0;
  int clientcount;
  struct timeval last;
  int clientconnectedport[FD_SETSIZE];
  gettimeofday(&last, NULL); // initailize time

  /* contorl server start */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&servaddr, 0,  sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
	bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++){
    client[i] = -1;			/* -1 indicates available entry */
    clientconnectedport[i] = -1;
  }
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
  /* contorl server end*/

  /* data sink server start*/
  listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(atoi(argv[1])+1);
  bind(listenfd2, (sockaddr*)&servaddr, sizeof(servaddr));
  listen(listenfd2, LISTENQ);
  maxfd = listenfd2;
  FD_SET(listenfd2, &allset);
  /* data sink server end*/



  for ( ; ; ) { /* select */
    rset = allset;		/* structure assignment */
    nready = select(maxfd+1, &rset, NULL, NULL, NULL);

    /* control server start*/
    if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
      clilen = sizeof(cliaddr);
      connfd = accept(listenfd, (sockaddr*)&cliaddr, &clilen);
      printf("(control server) client connected from %s: %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, ipstr, sizeof(ipstr)), ntohs(cliaddr.sin_port));

      for (i = 0; i < FD_SETSIZE; i++) {
        if (client[i] < 0) {
          client[i] = connfd;	/* save descriptor */
          clientconnectedport[i] = atoi(argv[1]);
          break;
        }
      }
      if (i == FD_SETSIZE) {
        printf("too many clients\n");
        exit(1);
      }

      FD_SET(connfd, &allset);	/* add new descriptor to set */
      if (connfd > maxfd) {
        maxfd = connfd;			/* for select */
      }
      if (i > maxi) {
        maxi = i;				/* max index in client[] array */
      }
      if (--nready <= 0 ) {
        continue;				/* no more readable descriptors */
      }
    }
    /* control server end*/

    /* data sink server start*/
    if(FD_ISSET(listenfd2, &rset)) {
      clilen = sizeof(cliaddr);
      connfd = accept(listenfd2, (sockaddr*)&cliaddr, &clilen);
      printf("(data sink server) client connected from %s: %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, ipstr, sizeof(ipstr)), ntohs(cliaddr.sin_port));

      for (i = 0; i < FD_SETSIZE; i++) {
        if (client[i] < 0) {
          client[i] = connfd;	/* save descriptor */
          clientconnectedport[i] = atoi(argv[1]) + 1;
          break;
        }
      }
      if (i == FD_SETSIZE) {
        printf("too many clients\n");
        exit(1);
      }
      FD_SET(connfd, &allset);	/* add new descriptor to set */
      if (connfd > maxfd) {
        maxfd = connfd;			/* for select */
      }
      if (i > maxi) {
        maxi = i;				/* max index in client[] array */
      }
      if (--nready <= 0 ) {
        continue;				/* no more readable descriptors */
      }
    }
    /* data sink server end*/

    // check all clients for data 
    for (i = 0; i <= maxi; i++) {
      if ( (sockfd = client[i]) < 0) {
        continue;
      }
      if (FD_ISSET(sockfd, &rset)) {
        if ( (n = read(sockfd, buf, MAXLINE)) == 0) {
          close(sockfd);
          FD_CLR(sockfd, &allset);
          client[i] = -1;
          clientconnectedport[i] = -1;
        } else {
          if(clientconnectedport[i] == atoi(argv[1])){
            if(memcmp(buf, "/reset\n", 7) == 0) {
              gettimeofday(&last, NULL);
              memset(timearray, 0, 50);
              memset(itoa, 0, itoalen);
              sprintf(itoa, "%lf", last.tv_sec+0.000001*last.tv_usec);
              strcat(timearray, itoa);

              memset(serverreply, 0, MAXLINE);
              strcat(serverreply, timearray);
              strcat(serverreply, " RESET ");
              memset(itoa, 0, itoalen);
              sprintf(itoa, "%d", count);
              strcat(serverreply, itoa);
              strcat(serverreply, "\n");
              write(sockfd, serverreply, strlen(serverreply));
              count = 0;
            } else if(memcmp(buf, "/ping\n", 6) == 0) {
              memset(serverreply, 0, MAXLINE);
              timestring(timearray, itoa);
              strcat(serverreply, timearray);
              strcat(serverreply, " PONG\n");
              write(sockfd, serverreply, strlen(serverreply));
            } else if(memcmp(buf, "/report\n", 8) == 0) {
              memset(serverreply, 0, MAXLINE);
              timestring(timearray, itoa);
              strcat(serverreply, timearray);
              strcat(serverreply, " REPORT ");
              memset(itoa, 0, itoalen);
              sprintf(itoa, "%d", count);
              strcat(serverreply, itoa);
              timediff(timediffarray, itoa, last);
              strcat(serverreply, " ");
              strcat(serverreply, timediffarray);
              strcat(serverreply, "s ");
              memset(itoa, 0, 50);
              sprintf(itoa, "%lf", 8.0*count/1000000.0/atoi(timediffarray));
              strcat(serverreply, itoa);
              strcat(serverreply, "Mbps\n");
              write(sockfd, serverreply, strlen(serverreply));
            } else if(memcmp(buf, "/clients\n", 9) == 0) {
              clientcount = 0;
              for(j=0; j<FD_SETSIZE; j++) {
                if(clientconnectedport[j] == atoi(argv[1]) + 1) {
                  clientcount++;
                }
              }
              memset(serverreply, 0, MAXLINE);
              timestring(timearray, itoa);
              strcat(serverreply, timearray);
              strcat(serverreply, " CLIENTS ");
              memset(itoa, 0, itoalen);
              sprintf(itoa, "%d", clientcount);
              strcat(serverreply, itoa);
              strcat(serverreply, "\n");
              write(sockfd, serverreply, strlen(serverreply));
            } 
          } else {
            count = count + n;
          }    
        }

        // no more readable descriptors
        if (--nready <= 0) {
          break;
        }
      }
    }
    memset(buf, 0, MAXLINE);
	}

  return 0;
}