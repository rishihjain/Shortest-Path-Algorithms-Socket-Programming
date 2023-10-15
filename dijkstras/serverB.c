#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SAP 21700
#define SBP 22700
#define AWSUPORT 23700
#define AWSTPORT 24700
#define LIP "127.0.0.1"

struct BQuery {
  double propSpeed;
  double tranSpeed;
  int arrayLen;
  double fileSize;
};

struct destLen {
  int dest;
  int len;
};

int main() {
	int sockFd;
	struct sockaddr_in mySock;

	// Prepare my socket.
	sockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockFd < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&mySock, 0, sizeof(mySock));

	mySock.sin_family = AF_INET;
	mySock.sin_addr.s_addr = inet_addr(LIP);
	mySock.sin_port = htons(SBP);

	// Bind the socket with the server address.
	int retval = bind(sockFd, (const struct sockaddr *)&mySock, sizeof(mySock));
	if (retval < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	// Prepare AWS socket.
	int WSockLen = 0;
	struct sockaddr_in WSock;
	memset(&WSock, 0, sizeof(WSock));

	WSock.sin_family = AF_INET;
	WSock.sin_port = htons(AWSUPORT);
	WSock.sin_addr.s_addr = inet_addr(LIP);

	printf("The Server B is up and running using UDP on port %d.\n", SBP);

	while (true) {
		// Receive info from AWS.
		struct BQuery awsToBMsg;
		recvfrom(sockFd, &awsToBMsg, sizeof(awsToBMsg), 0,
						(struct sockaddr *) &WSock, &WSockLen);

		// Receive the destLenArray.
    struct destLen destLenArray[awsToBMsg.arrayLen];
    recvfrom(sockFd, destLenArray, sizeof(destLenArray), 0,
      (struct sockaddr *)&WSock, &WSockLen);

		printf("The Server B has received data for calculation:\n");
		printf("* Propogation speed: %.2f km/s;\n", awsToBMsg.propSpeed);
		printf("* Transmission speed %.2f Bytes/s;\n", awsToBMsg.tranSpeed);

		for (size_t i = 0; i < awsToBMsg.arrayLen; i++) {
			printf("* Path length for destination %d: %d;\n", destLenArray[i].dest, \
					destLenArray[i].len);
		}

		// Do calculation.
		double tranTime[awsToBMsg.arrayLen];
		double propTime[awsToBMsg.arrayLen];
		double delay[awsToBMsg.arrayLen];

		for (size_t i = 0; i < awsToBMsg.arrayLen; i++) {
			tranTime[i] = (awsToBMsg.fileSize / 8) / awsToBMsg.tranSpeed;
			propTime[i] = destLenArray[i].len / awsToBMsg.propSpeed;
			delay[i] = tranTime[i] + propTime[i];
		}

		printf("The Server B has finished the calculation of the delays:\n");
		printf("----------------------\n");
		printf("Destination    Delay\n");
		printf("----------------------\n");
		for (size_t i = 0; i < awsToBMsg.arrayLen; i++) {
			printf("%d             %.2f\n", destLenArray[i].dest, delay[i]);
		}
		printf("----------------------\n");

		// Send calculation result to AWS.
		sendto(sockFd, tranTime, sizeof(tranTime), 0,
			(struct sockaddr *) &WSock, sizeof(WSock));
		sendto(sockFd, propTime, sizeof(propTime), 0,
			(struct sockaddr *) &WSock, sizeof(WSock));
		sendto(sockFd, delay, sizeof(delay), 0,
			(struct sockaddr *) &WSock, sizeof(WSock));

		printf("The Server B has finished sending the output to AWS\n");


	}
}
