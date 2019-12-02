#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

// Error function used for reporting issues
void error(const char *msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int listenSocketFD;
	int establishedConnectionFD;
	int portNumber;
	int charsRead;
	
	pid_t spawnPid = -5;
	int sp_child_exit = -5;

	char buffer[256];
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress;
	struct sockaddr_in clientAddress;

	if (argc < 2) {
		fprintf(stderr,"USAGE: %s port\n", argv[0]);
		exit(1);
	} // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); 								// Convert port to an integer from a string
	serverAddress.sin_family = AF_INET; 				// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	// Connect socket to port
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR on binding");
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Loop indefinitely
	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect, clientAddress will hold client data
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) {
			error("ERROR on accept");
		}

		spawnPid = fork();

		switch(spawnPid) {
			case -1:
				error("Hull Breach");
				break;

			case 0:
				// Get the message from the client and display it
				memset(buffer, '\0', 256);
				charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
				if (charsRead < 0) error("ERROR reading from socket");
				printf("SERVER: I received this from the client: \"%s\"\n", buffer);

				// Send a Success message back to the client
				charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
				if (charsRead < 0) {
					error("ERROR writing to socket");
				}
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				break;

			default:
				spawnPid = waitpid(-1, &sp_child_exit, WNOHANG);

				break;
		}
	}
	close(listenSocketFD); // Close the listening socket
	return 0;
}
