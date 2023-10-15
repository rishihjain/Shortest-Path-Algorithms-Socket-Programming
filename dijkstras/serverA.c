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

struct cityMap {
  char mapid;
  double propSpeed;
  double tranSpeed;
  int labelArray[10];  // Stores the map from index 0-9 to label index.
  int vertexNum;  // Number of valid labels in labelArray.
  int edgeNum;  // Number of edges.
  int adjMatrix[10][10]; // Graph's adjacency matrix.
};

struct AQuery {
  char mapid;
  int srcVertex;
};

struct destLen {
  int dest;
  int len;
};

bool ifVertexInLabelArray(int labelArray[], int label, int vertexNum) {
  for (size_t i = 0; i < vertexNum; i++) {
    if (labelArray[i] == label) {
      return true;
    }
  }
  return false;
}

int minOfTwo(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}


void splitLineToArray(char *line, int array[3]) {
  int arrayIndex = 0;
  char *end = line;
  while(*end) {
    if (arrayIndex > 3) {
      perror("splitLineToArray goes wrong");
      exit(1);
    }
    array[arrayIndex] = strtol(line, &end, 10);
    arrayIndex += 1;
    while (*end == ' ') {
        end += 1;
    }
    line = end;
  }
}

int mapConstruction(struct cityMap* cityMaps) {
  int retval;
  FILE *mapFile = fopen("map.txt", "r");
  if (mapFile == NULL) {
    perror("open map.txt fail");
    exit(1);
  }
  char *line = NULL;
  size_t linelen = 0;
  int mapCount = -1;
  while (getline(&line, &linelen, mapFile) != -1) {
    line[strlen(line) - 2] = '\0';
    if (isalpha(line[0])) {
      mapCount += 1;
      cityMaps[mapCount].mapid = line[0];
      getline(&line, &linelen, mapFile);

      line[strlen(line) - 2] = '\0';
      cityMaps[mapCount].propSpeed = atof(line);

      getline(&line, &linelen, mapFile);
      line[strlen(line) - 2] = '\0';
      cityMaps[mapCount].tranSpeed = atof(line);
    }

    else{
      int info[3];
      splitLineToArray(line, info);
      int label1 = info[0];
      int label2 = info[1];
      int distance = info[2];
      int vertex1, vertex2;

      if ( !ifVertexInLabelArray(cityMaps[mapCount].labelArray, label1,
          cityMaps[mapCount].vertexNum)) {
        cityMaps[mapCount].labelArray[(cityMaps[mapCount].vertexNum)] = label1;
        cityMaps[mapCount].vertexNum += 1;
      }
      if ( !ifVertexInLabelArray(cityMaps[mapCount].labelArray, label2,
          cityMaps[mapCount].vertexNum)) {
        cityMaps[mapCount].labelArray[cityMaps[mapCount].vertexNum] = label2;
        cityMaps[mapCount].vertexNum += 1;
      }

      for (size_t i = 0; i < cityMaps[mapCount].vertexNum; i++) {
        if (cityMaps[mapCount].labelArray[i] == label1) {
          vertex1 = i;
        }
        if (cityMaps[mapCount].labelArray[i] == label2) {
          vertex2 = i;
        }
      }

      cityMaps[mapCount].adjMatrix[vertex1][vertex2] = distance;
      cityMaps[mapCount].adjMatrix[vertex2][vertex1] = distance;
      cityMaps[mapCount].edgeNum += 1;
    }
  }

  free(line);
  return mapCount + 1;
}

void mapConstructionPrint(int mapNum, struct cityMap* cityMaps) {
  // Print the on screen message for map construction.
  printf("The Server A has constructed a list of %d maps: \n", mapNum);
  printf("------------------------------\n");
  printf("Map ID    Num Vertices    Num Edges\n");
  for (size_t i = 0; i < mapNum; i++) {
    printf("%c        %d              %d\n", cityMaps[i].mapid,
      cityMaps[i].vertexNum, cityMaps[i].edgeNum);
  }
  printf("------------------------------\n");
}

void pathFindingAndPrinting(int srcLabel, struct cityMap* city,
    struct destLen* destLenArray) {
  int srcVertex = -1;
  for (size_t i = 0; i < city->vertexNum; i++) {
    if (srcLabel == city->labelArray[i]) {
      srcVertex = i;
    }
  }
  if (srcVertex == -1) {
    perror("path finding locating srcVertex fail");
    exit(1);
  }

  int finishedNodesCount = 0;
  int finishedNodes[city->vertexNum]; // 0 is not finished, 1 is finished.

  int currentLen[city->vertexNum];

  for (size_t i = 0; i < city->vertexNum; i++) {
    finishedNodes[i] = 0;
    currentLen[i] = -1;
  }

  currentLen[srcVertex] = 0;
  while (finishedNodesCount != city->vertexNum) {
    // Find smallest unfinished node and length.
    int smallestNode;
    int smallestLen;
    bool initialized = false;
    for (size_t i = 0; i < city->vertexNum; i++) {
      if ( !finishedNodes[i]) {
        if (currentLen[i] != -1) {
          if (!initialized) {
            smallestLen = currentLen[i];
            smallestNode = i;
            initialized = true;
          }
          else {
            if (currentLen[i] < smallestLen) {
              smallestLen = currentLen[i];
              smallestNode = i;
            }
          }
        }
      }
    }
    finishedNodes[smallestNode] = 1;
    finishedNodesCount += 1;
    currentLen[smallestNode] = smallestLen;

    // Find adjacent nodes and distance, and update their currentLen.
    for (size_t i = 0; i < city->vertexNum; i++) {
      // i is the adjacent node.
      int distance = city->adjMatrix[smallestNode][i];
      if (distance != -1) {
        // If node i is unvisited.
        if (currentLen[i] == -1) {
          currentLen[i] = smallestLen + distance;
        }
        // If node i is visited before.
        else {
          int len = currentLen[i];
          currentLen[i] = minOfTwo((smallestLen + distance), len);
        }
      }
    }
  }

  // Clean the result and remove source node info.
  bool gap = false;
  for (size_t i = 0; i < city->vertexNum; i++) {
    if (srcVertex != i) {
      if (gap == true) {
        destLenArray[i-1].dest = city->labelArray[i];
        destLenArray[i-1].len = currentLen[i];
      }
      else {
        destLenArray[i].dest = city->labelArray[i];
        destLenArray[i].len = currentLen[i];
      }
    }
    if (srcVertex == i) {
      gap = true;
    }
  }

  
  int i = 1;
  struct destLen temp;
  while (i < (city->vertexNum) - 1) {
    int j = i;
    while (j > 0 && destLenArray[j-1].dest > destLenArray[j].dest) {
      temp = destLenArray[j];
      destLenArray[j] = destLenArray[j-1];
      destLenArray[j-1] = temp;
    }
    i += 1;
  }

  printf("The Server A has identified the following shortest paths: \n");
  printf("----------------------\n");
  printf("Destination    Min Length\n");
  printf("----------------------\n");
  for (size_t i = 0; i < city->vertexNum - 1; i++) {
    printf("%d              %d\n",destLenArray[i].dest, destLenArray[i].len);
  }
  printf("----------------------\n");
}


int main() {
  int sockFd;
  struct sockaddr_in mySock;

  // Creating socket file descriptor.
  sockFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockFd < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&mySock, 0, sizeof(mySock));

  mySock.sin_family = AF_INET;
  mySock.sin_addr.s_addr = inet_addr(LIP);
  mySock.sin_port = htons(SAP);

  // Bind the socket with the server address.
  int retval = bind(sockFd, (const struct sockaddr *)&mySock, sizeof(mySock));
  if (retval < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  printf("The server A is up and running using UDP on port %d\n", SAP);

  struct cityMap cityMaps[100];
  for (size_t i = 0; i < 100; i++) {
    cityMaps[i].vertexNum = 0;
    cityMaps[i].edgeNum = 0;
    for (size_t j = 0; j < 10; j++) {
      cityMaps[i].labelArray[j] = -1;
      for (size_t k = 0; k < 10; k++) {
        cityMaps[i].adjMatrix[j][k] = -1;
      }
    }
  }
  int mapNum = mapConstruction(cityMaps);
  mapConstructionPrint(mapNum, cityMaps);

  // Prepare AWS socket.
  int WSockLen = 0;
  struct sockaddr_in WSock;
  memset(&WSock, 0, sizeof(WSock));

  WSock.sin_family = AF_INET;
  WSock.sin_port = htons(AWSUPORT);
  WSock.sin_addr.s_addr = inet_addr(LIP);

  while (true) {
    // Receive info from AWS.
    struct AQuery awsToAMsg;
    recvfrom(sockFd, &awsToAMsg, sizeof(awsToAMsg), 0,
            (struct sockaddr *) &WSock, &WSockLen);
    printf("The Server A has received input for finding shortest paths: starting "
            "vertex %d of map %c\n", awsToAMsg.srcVertex, awsToAMsg.mapid);
    for (size_t i = 0; i < mapNum; i++) {
      // Check which map corresponds to the sent mapid.
      if (cityMaps[i].mapid == awsToAMsg.mapid) {
        struct destLen destLenArray[cityMaps[i].vertexNum-1];
        pathFindingAndPrinting(awsToAMsg.srcVertex, &(cityMaps[i]), destLenArray);

        // Send the number of destination nodes.
        int destNum = cityMaps[i].vertexNum - 1;
        sendto(sockFd, &destNum, sizeof(destNum), 0,
          (struct sockaddr *) &WSock, sizeof(WSock));
        // Send the dest and len pair.
        sendto(sockFd, destLenArray, sizeof(destLenArray), 0,
          (struct sockaddr *) &WSock, sizeof(WSock));
        // Send propSpeed.
        double propSpeed = cityMaps[i].propSpeed;
        sendto(sockFd, &propSpeed, sizeof(propSpeed), 0,
          (struct sockaddr *) &WSock, sizeof(WSock));
        // Send tranSpeed.
        double tranSpeed = cityMaps[i].tranSpeed;
        sendto(sockFd, &tranSpeed, sizeof(tranSpeed), 0,
          (struct sockaddr *) &WSock, sizeof(WSock));

        printf("The Server A has sent shortest paths to AWS.\n");
        break;
      }
    }
  }

  close(sockFd);
  return 0;
}
