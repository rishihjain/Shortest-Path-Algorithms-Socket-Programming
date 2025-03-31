## Network Delay Simulation Using Socket Programming

# Overview

This project simulates a distributed network system that calculates the shortest path between nodes in a network and determines the transmission delay. It utilizes socket programming in C with both TCP and UDP protocols for communication between different components.

# Components

The system consists of four key components:

Server A: Computes the shortest path using Dijkstra's algorithm.

Server B: Computes the transmission and propagation delays based on path length and network speed.

AWS (Advanced Web Server): Acts as the intermediary, forwarding data between the client, Server A, and Server B.

Client: Sends a request for shortest path and delay calculations and receives the final results.

File Structure

serverA.c - Implements shortest path computation using Dijkstra’s algorithm.

serverB.c - Computes network delays.

aws.c - Handles communication between client and servers.

client.c - Sends requests and receives results.

map.txt - Contains predefined network maps with nodes, edges, and transmission properties.

Communication Flow

Client to AWS (TCP): The client sends a request with a map ID, source node, and file size.

AWS to Server A (UDP): AWS forwards the request to Server A.

Server A to AWS (UDP): Server A computes the shortest paths and sends results back.

AWS to Server B (UDP): AWS forwards shortest path data to Server B for delay computation.

Server B to AWS (UDP): Server B computes and returns delay values.

AWS to Client (TCP): AWS consolidates the data and sends the final result back to the client.

Installation & Compilation

Clone the repository or download the source files.

Compile each component using GCC:

gcc -o serverA serverA.c
gcc -o serverB serverB.c
gcc -o aws aws.c
gcc -o client client.c

Execution Steps

Start Server A:

./serverA &

Start Server B:

./serverB &

Start AWS:

./aws &

Run Client with parameters:

./client <MapID> <SourceNode> <FileSize>

Example:

./client A 1 1000

Expected Output

The client receives the shortest paths and delay values for all reachable nodes and displays them in a structured format.

Technologies Used

Programming Language: C

Protocols: TCP (Client-AWS), UDP (AWS-Servers)

Algorithms: Dijkstra’s Algorithm for shortest path calculation
