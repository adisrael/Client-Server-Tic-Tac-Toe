# Client-Server-Tic-Tac-Toe

2 Player Tic-Tac-Toe
====

Date: Nov/Dec 2018

### Client - Server style application for playing Tic-Tac-Toe and chatting between 2 client players.

* C Programming
* Socket Posix API
* Clients behave as dumb clients, as the server controls the complete flow of the program
* The program works with a Standard Protocol that considers binary messages consistent of:
  * MessageType ID (1 Byte)
  * Payload Size (1 Byte)
  * Payload (Variable size)

Project for the course IIC2333 - Operative Systems & Networks at Pontificia Universidad de Chile (PUC).

To run the program:
 * Copy the Repository
 * Compile the code using the Makefile - ```make``` (you need gcc to compile)  
 * Run the server:

 ```./server -i <IP Address> -p <Port Number>```

 * Run the clients:

 ```./client -i <Server IP Address> -p <Port Number>```

 (if you are running the program locally use 0.0.0.0 as IP Address)

