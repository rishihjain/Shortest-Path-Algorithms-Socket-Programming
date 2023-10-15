#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SAP 21700  // Server A port number.
#define SBP 22700  // Server B port number.
#define AWSUPORT 23700  // AWS UDP port number.
#define AWSTPORT 24700  // AWS TCP port number.
#define LIP "127.0.0.1"  // Local host IP address.

// Query structure received from client.
struct clientQuery {
  char mapid;
  int srcVertex;
  double fileSize;
};

// Query structure sent to server A.
struct AQuery {
  char mapid;
  int srcVertex;
};

struct destLen {
  int dest;
  int len;
};

struct BQuery {
  double propSpeed;
  double tranSpeed;
  int arrayLen;
  double fileSize;
};

int main() {
	int retval;
	printf("The AWS is up and running.\n");

	// Create TCP socket.
	int tcpFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcpFd < 0) {
		perror("tcpFd failed");
		exit(1);
	}

	struct sockaddr_in tcpSock;
	memset(&tcpSock, 0, sizeof(tcpSock));

	tcpSock.sin_family = AF_INET;
	tcpSock.sin_port = htons(AWSTPORT);
	tcpSock.sin_addr.s_addr = inet_addr(LIP);

	retval = bind(tcpFd, (struct sockaddr *)&tcpSock, sizeof(tcpSock));
	if (retval < 0) {
		perror("aws tcp bind fail");
		exit(1);
	}

	retval = listen(tcpFd, 15);
	if (retval < 0) {
		perror("listen fail");
	}

	// Prepare cleint's TCP communication.
	struct sockaddr_in clientSock;
	socklen_t clientSockLen;
	clientSockLen = sizeof(clientSock);
	memset(&clientSock, 0, sizeof(clientSock));


	struct clientQuery clientToAWSMsg;
	int clientToAWSMsgLen = 100;

  // Create UDP socket.
  int udpFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udpFd < 0) {
    perror("udpFd fail");
    exit(1);
  }

  struct sockaddr_in udpSock;
  memset(&udpSock, 0, sizeof(udpSock));

  udpSock.sin_family = AF_INET;
  udpSock.sin_port = htons(AWSUPORT);
  udpSock.sin_addr.s_addr = inet_addr(LIP);

  // Bind UDP socket.
  retval = bind(udpFd, (struct sockaddr *)&udpSock, sizeof(udpSock));
  if (retval < 0) {
    perror("aws udp bind fail");
    exit(1);
  }

  // Prepare UDP socket connected to A.
  struct sockaddr_in sockA;
  int sockALen;
  memset(&sockA, 0, sizeof(sockA));
  sockA.sin_family = AF_INET;
  sockA.sin_port = htons(SAP);
  sockA.sin_addr.s_addr = inet_addr(LIP);

  // Prepare UDP socket connected to B.
  struct sockaddr_in sockB;
  int sockBLen;
  memset(&sockB, 0, sizeof(sockB));
  sockB.sin_family = AF_INET;
  sockB.sin_port = htons(SBP);
  sockB.sin_addr.s_addr = inet_addr(LIP);


  while (true) {
    // Accept the connection from the client.
    int childSockFd = accept(tcpFd, (struct sockaddr *)&clientSock, &clientSockLen);
    if (childSockFd <= 0) {
      perror("accept failed");
      exit(1);
    }

  	retval = recv(childSockFd, &clientToAWSMsg, sizeof(clientToAWSMsg), 0);
  	if (retval < 0) {
  		perror("recv fail");
  		exit(1);
  	}

    printf("The AWS has received map ID %c, start vertex %d and file size %.0lf "
        "from the client using TCP over port %d\n",\
        clientToAWSMsg.mapid, clientToAWSMsg.srcVertex, \
        clientToAWSMsg.fileSize, AWSTPORT);

    // -----------------Finished with client, begin server A.------------------

    // Send query to A.
    struct AQuery awsToAMsg;
    awsToAMsg.mapid = clientToAWSMsg.mapid;
    awsToAMsg.srcVertex = clientToAWSMsg.srcVertex;

    sendto(udpFd, &awsToAMsg, sizeof(awsToAMsg), 0,
      (struct sockaddr *)&sockA, sizeof(sockA));

    printf("The AWS has sent map ID and starting vertex to server A using "
          "UDP over port %d\n", AWSUPORT);

    // Receive the number of destination nodes.
    int arrayLen;
    recvfrom(udpFd, &arrayLen, sizeof(arrayLen), 0,
      (struct sockaddr *)&sockA, &sockALen);
    // Receive the destLenArray.
    struct destLen destLenArray[arrayLen];
    recvfrom(udpFd, destLenArray, arrayLen * sizeof(struct destLen), 0,
      (struct sockaddr *)&sockA, &sockALen);
    // Receive propSpeed.
    double propSpeed;
    recvfrom(udpFd, &propSpeed, sizeof(propSpeed), 0,
      (struct sockaddr *)&sockA, &sockALen);
    // Receive tranSpeed.
    double tranSpeed;
    recvfrom(udpFd, &tranSpeed, sizeof(tranSpeed), 0,
      (struct sockaddr *)&sockA, &sockALen);

    printf("The AWS has received shortest path from server A: \n");
    printf("----------------------\n");
    printf("Destination    Min Length\n");
    printf("----------------------\n");
    for (size_t i = 0; i < arrayLen; i++) {
      printf("%d              %d\n",destLenArray[i].dest, destLenArray[i].len);
    }
    printf("----------------------\n");

    // -----------------Finished with server A, begin server B.----------------

    // Send query to B.
    struct BQuery awsToBMsg;
    awsToBMsg.propSpeed = propSpeed;
    awsToBMsg.tranSpeed = tranSpeed;
    awsToBMsg.arrayLen = arrayLen;
    awsToBMsg.fileSize = clientToAWSMsg.fileSize;

    sendto(udpFd, &awsToBMsg, sizeof(awsToBMsg), 0,
      (struct sockaddr *)&sockB, sizeof(sockB));

    sendto(udpFd, destLenArray, sizeof(destLenArray), 0,
      (struct sockaddr *)&sockB, sizeof(sockB));

    printf("The AWS has sent path length, propagation speed and transmission "
      "speed to server B using UDP over port %d.\n", AWSUPORT);

    // Receive calculation result from B.
    double tranTime[awsToBMsg.arrayLen];
    double propTime[awsToBMsg.arrayLen];
    double delay[awsToBMsg.arrayLen];

    recvfrom(udpFd, tranTime, sizeof(tranTime), 0,
      (struct sockaddr *)&sockB, &sockBLen);
    recvfrom(udpFd, propTime, sizeof(propTime), 0,
      (struct sockaddr *)&sockB, &sockBLen);
    recvfrom(udpFd, delay, sizeof(delay), 0,
      (struct sockaddr *)&sockB, &sockBLen);

    printf("The AWS has received delays from server B: \n");
    printf("----------------------\n");
    printf("Destination    Tt      Tp      Delay\n");
    printf("----------------------\n");
    for (size_t i = 0; i < arrayLen; i++) {
      printf("%d            %.2f   %.2f   %.2f\n",
        destLenArray[i].dest, tranTime[i], propTime[i], delay[i]);
    }
    printf("----------------------\n");

    // -----------------Finished with server B, begin client.------------------

    // Send number of dest first, which is arrayLen.
    send(childSockFd, &arrayLen, sizeof(arrayLen), 0);
    send(childSockFd, destLenArray, sizeof(destLenArray), 0);
    send(childSockFd, tranTime, sizeof(tranTime), 0);
    send(childSockFd, propTime, sizeof(propTime), 0);
    send(childSockFd, delay, sizeof(delay), 0);
    printf("The AWS has sent calculated delay to client using TCP over "
      "port %d. \n", AWSTPORT);

  }

	close(udpFd);
	close(tcpFd);
}
