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
#define CHARMAX 70000
#define CHARBYTE 6


void encryptBuf(char *key, char *text, char *cipher) {
	int i, c, t, k;

	for (i = 0; i < strlen(text) - 1; i++) {
		t = text[i] - 64;				// Adjust text char to 0-26
		k = key[i] - 64;				// Adjust key char to 0-26
		if (k == -32) {k = 0;}	// If newline val from 26 - 65
		if (t == -32) {t = 0;}
		c = (t + k) % 27;							// Add key and text value together
		if (c == 0) {c = 32;}	// If equal to newline value
		else {c += 64;}					// Else add back 65 to get ASCII values
		cipher[i] = c;					// read to array
	}
	cipher[i + 1] = '\n';
}

// Error function used for reporting issues
void error(const char *msg) {
	perror(msg);
	exit(1);
}

int recvMSG(int connectionFD, char* buffer, int bSize, int flag) {
	int charsRead, charsTotal = 0;
	while(!strcspn(buffer, "\n")) {
		charsRead = recv(connectionFD, buffer + charsTotal, bSize, flag);
		if (charsRead <= 0) {
			error("SERVER: ERROR reading from socket");
			break;
		}
		charsTotal += charsRead;
	}
	return charsTotal;
}

int recvMsgChar(int connectionFD, char* buffer, int bSize, int msgChars, int flag) {
	int charsRead, charsTotal = 0;
	while(charsTotal < msgChars) {
		charsRead = recv(connectionFD, buffer + charsTotal, bSize, flag);
		if (charsRead < 0) {
			error("SERVER: ERROR reading from socket");
			break;
		}
		charsTotal += charsRead;
	}
	return charsTotal;
}

int sendMSG(int connectionFD, char* buffer, int flag) {
	int charsWritten, charsTotal = 0;
	// block until all data is sent
	while (charsTotal < strlen(buffer)) {
		charsWritten = send(connectionFD, buffer + charsTotal, strlen(buffer), flag);
		if (charsWritten < 0) {
			error("SERVER: ERROR writing to socket");
		}
		if (charsWritten == 0) {
			break;
		}
		charsTotal += charsWritten;
	}
	return charsTotal;
}

int main(int argc, char *argv[]) {
	int listenSocketFD;
	int establishedConnectionFD;
	int portNumber;
	int charsRead;
	int charsWritten;							// char[] size sent to otp_enc_d process

	pid_t spawnPid = -5;
	int spChildExit = -5;

	int textSize;
	int keySize;
	int ktSize;										// stores keyBuf + textBuf recv() size

	char* encStr = "otp_enc\n";
	char* encdStr = "otp_enc_d\n";

	char buffer[CHARMAX];			// store all recv() data
	char cipher[CHARMAX];			// store ciphertext
	char textBuf[CHARMAX];		// stores text buffer
	char keyBuf[CHARMAX];			// stores key buffer
	char buffByte[CHARBYTE];	// stores key buffer size from client

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
				memset(buffer, '\0', sizeof(buffer)); 					// Clear out the buffer again for reuse
				charsRead = recvMSG(establishedConnectionFD, buffer, CHARMAX, 0);				// Receive confirmation from client
				//printf("SERVER rec : %s", buffer);
				//printf("SERVER enc : %s", encStr);

				if (strcmp(buffer, encStr) != 0) {															// If process is not otp_enc
					charsWritten = sendMSG(establishedConnectionFD, "error", 0);	// Send error text to client
					close(establishedConnectionFD);																// close and exit process
					exit(0);
					break;
				}
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0); 	// Send confirmation to otp_enc
				/*****************************
				** Receive key size from otp_enc
				******************************/
				memset(buffer, '\0', CHARMAX); 																		// Clear out the buffer again for reuse
				charsRead = recvMSG(establishedConnectionFD, buffer, CHARMAX, 0);	// Receive key from otp_enc
				buffer[strcspn(buffer, "\n")] = '\0';
				keySize = atoi(buffer);
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0);				// SEnd text to otp_enc_d
				/*****************************
				** Receive key from otp_enc
				******************************/
				memset(keyBuf, '\0', CHARMAX); 																		// Clear out the buffer again for reuse
				charsRead = recvMsgChar(establishedConnectionFD, keyBuf, CHARMAX, keySize, 0);	// Receive key from otp_enc
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0);				// SEnd text to otp_enc_d
				// printf("SERVER: key size: %d, keyBuf size: %lu\n", keySize, strlen(keyBuf));
				/*****************************
				** Receive text size from otp_enc
				******************************/
				memset(buffer, '\0', CHARMAX); 																		// Clear out the buffer again for reuse
				charsRead = recvMSG(establishedConnectionFD, buffer, CHARMAX, 0);	// Receive key from otp_enc
				buffer[strcspn(buffer, "\n")] = '\0';
				textSize = atoi(buffer);
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0);
				/*****************************
				** Receive text from otp_enc
				******************************/
				memset(textBuf, '\0', CHARMAX);																// Clear out the textBuf for reuse
				charsRead = recvMsgChar(establishedConnectionFD, textBuf, CHARMAX, textSize, 0);			// Receive plain text from otp_enc
				// printf("SERVER: text size: %d, textBuf size: %lu\n", textSize, strlen(textBuf));
				encryptBuf(keyBuf, textBuf, cipher);													// Encrypt ciphertext

				// printf("SERVER: texBuf= %s\n", textBuf);
				// printf("SERVER: cipher size= %lu\n", strlen(cipher));
				// printf("SERVER: keyBuf= %s\n", keyBuf);
				charsWritten = sendMSG(establishedConnectionFD, cipher, 0); 	// Send ciphertext back

				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				exit(0);
				break;

			default:
				spawnPid = waitpid(-1, &spChildExit, WNOHANG);
				break;
		}

		while ((spawnPid = waitpid(-1, &spChildExit, WNOHANG)) > 0) {
			fflush(stdout);
		}
		//printf("after switch, pid = %d\n", spawnPid);
	}
	close(listenSocketFD); // Close the listening socket
	return 0;
}
