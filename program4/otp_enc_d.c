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

// Boolean
#define TRUE 1
#define FALSE 0
#define CHARRMAX 65535
#define CHARBYTE 6
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
	int spChildExit = -5;

	int keyRead;
	int textPos;
	int ktSize;								// stores keyBuf + textBuf recv() size
	char buffer[CHARRMAX];			// store all recv() data
	char textBuf[CHARRMAX];			// stores text buffer
	char keyBuf[CHARRMAX];				// stores key buffer
	char buffByte[CHARBYTE];

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
		keyRead = FALSE;
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect, clientAddress will hold client data
		while ((spawnPid = waitpid(-1, &spChildExit, WNOHANG)) > 0) {
			printf("background pid %d is done: ", spawnPid);
			fflush(stdout);
		}
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
				// Get byte Size of textBuf + keyBuf files
				memset(buffByte, '\0', CHARBYTE);
				while (1) {
					charsRead = recv(establishedConnectionFD, buffByte, CHARBYTE, 0);
					if(strcspn(buffByte, "\n")) {
						break;
					}
				}
				ktSize = atoi(buffByte);


				charsRead = send(establishedConnectionFD, buffByte, strlen(buffByte), 0); // Send success back
				if (charsRead < 0) {
					error("ERROR writing to socket");
				}

				// Get the message from the client and display it
				memset(buffer, '\0', CHARRMAX);
				int totalChar;
				int idx;
				while(1) {
					// receive file
					charsRead = recv(establishedConnectionFD, buffer, CHARRMAX, 0);
					if (charsRead < 0) {
						error("ERROR reading from socket");
					}
					totalChar += charsRead;
					// set index of where text file starts
					if (keyRead == FALSE && (idx = strcspn(buffer, "\n"))) {
						textPos = idx;
						keyRead = TRUE;
					}
					// if all bytes have been received
					if (totalChar == ktSize && keyRead == TRUE) {
						break;
					}
					printf("in while");
				}

				/************************
				** Parse Key-Text Buffer
				*************************/
				memset(keyBuf, '\0', CHARRMAX);
				memset(textBuf, '\0', CHARRMAX);
				memcpy(keyBuf, &buffer[0], textPos);
				memcpy(textBuf, &buffer[textPos + 1], strlen(buffer));
				printf("SERVER: keyBuf= %s\n", keyBuf);
				printf("SERVER: texBuf= %s\n", textBuf);

				//while (1) {} read line
				// Send a Success message back to the client
				charsRead = send(establishedConnectionFD, "I am the server, and I got your message", 39, 0); // Send success back
				if (charsRead < 0) {
					error("ERROR writing to socket");
				}
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				exit(0);
				break;

			default:
				spawnPid = waitpid(-1, &spChildExit, WNOHANG);
				break;
		}
		//printf("after switch, pid = %d\n", spawnPid);
	}
	close(listenSocketFD); // Close the listening socket
	return 0;
}
