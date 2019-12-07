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

#define TRUE 1
#define FALSE 0
#define CHARMAX 70000

/***********************************************
** Function: Error Handler
** Description: Sends message to stderr and exits with status
************************************************/
void error(const char *msg, int status) {
	perror(msg);
	exit(status);
}
/***********************************************
** Function: Encrypt Buffer
** Description: Creates ciphertext from key and text
** Prerequisites: key[] & text[] contain valid arrays where key >= text, cipher[] NULL
************************************************/
void encryptBuf(char *key, char *text, char *cipher) {
	int i, c, t, k;
	for (i = 0; i < strlen(text) - 1; i++) {
		t = text[i] - 64;				// Adjust text char to 0-26
		k = key[i] - 64;				// Adjust key char to 0-26
		if (k == -32) {k = 0;}	// If newline val from 26 - 65
		if (t == -32) {t = 0;}
		c = (t + k) % 27;				// Add key and text value together mod 27
		if (c == 0) {c = 32;}		// If equal to newline value
		else {c += 64;}					// Else add back 65 to get ASCII values
		cipher[i] = c;					// read to array
	}
	cipher[i + 1] = '\n';
}
/***********************************************
** Function: Receive Message
** Descripton: Receives confirmation or buffer size of key/text from otp_enc
** Prerequisite: buffer is memset to null, bSize contains positive integer, flag contains positive integer. Called within child of spawnPid
************************************************/
int recvMSG(int connectionFD, char* buffer, int bSize, int flag) {
	int charsRead, charsTotal = 0;
	// While buffer doesn't contain endline
	while(!strcspn(buffer, "\n")) {
		charsRead = recv(connectionFD, buffer + charsTotal, bSize, flag);
		if (charsRead <= 0) {
			error("SERVER: ERROR reading from socket", 1);
			break;
		}
		charsTotal += charsRead;	// add to offset next receive
	}
	return charsTotal;
}
/***********************************************
** Function: Receive Message Character
** Descripton: Receives buffer of key/text and loops for msgChars from otp_enc
** Prerequisite: buffer is memset to null. bSize, msgChars, flag must contain positive integers. Called within child of spawnPid, recvMsg() must be called prior to set size of msgChars
************************************************/
int recvMsgChar(int connectionFD, char* buffer, int bSize, int msgChars, int flag) {
	int charsRead, charsTotal = 0;
	// block until all data is received
	while(charsTotal < msgChars) {
		charsRead = recv(connectionFD, buffer + charsTotal, bSize, flag);
		if (charsRead < 0) {
			error("SERVER: ERROR reading from socket", 1);
			break;
		}
		charsTotal += charsRead;	// add to offset next receive
	}
	return charsTotal;
}
/***********************************************
** Function: Send Message
** Descripton: Sends confirmation/encrypted text back to otp_enc
** Prerequisite: buffer is memset to null, bSize contains positive integer, flag contains positive integer. Called within child of spawnPid
************************************************/
int sendMSG(int connectionFD, char* buffer, int flag) {
	int charsWritten, charsTotal = 0;
	// block until all data is sent
	while (charsTotal < strlen(buffer)) {
		charsWritten = send(connectionFD, buffer + charsTotal, strlen(buffer), flag);
		if (charsWritten < 0) {
			error("SERVER: ERROR writing to socket", 1);
		}
		if (charsWritten == 0) {
			break;
		}
		charsTotal += charsWritten; 	// add to offset next send
	}
	return charsTotal;
}
/***********************************************
** Function: Main
** Prerequisite: args[] contains {command, port}
************************************************/
int main(int argc, char *argv[]) {
	int listenSocketFD;						// socket file descriptor
	int establishedConnectionFD;	// file descriptor to connection
	int portNumber;								// port to listen on
	int charsRead;								// char[] size read from otp_enc process
	int charsWritten;							// char[] size sent to otp_enc process

	char* encStr = "otp_enc\n";		// Used for verification of encrypt process

	char buffer[CHARMAX];		// store all recv() data
	char cipher[CHARMAX];		// store ciphertext
	char textBuf[CHARMAX];	// stores text buffer
	char keyBuf[CHARMAX];		// stores key buffer
	int textSize;						// holds size of textBuf for recvMsgChar
	int keySize;						// holds size of keyBuf for recvMsgChar

	pid_t spawnPid = -5;	// holds spawn child process
	int spChildExit = -5;	// holds exit method of spawn child

	socklen_t sizeOfClientInfo;					// Holds sizeof() client address
	struct sockaddr_in serverAddress;		// Holds address of otp_enc_d process plus settings
	struct sockaddr_in clientAddress;		// Holds address of otp_enc process plus settings

	/******************************
	** Check Args
	*******************************/
	if (argc < 2) {
		fprintf(stderr,"USAGE: %s port\n", argv[0]);
		exit(1);
	}
	/******************************
	** Set up the address struct for this process and socket
	*******************************/
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); 								// Convert port to an integer from a string
	serverAddress.sin_family = AF_INET; 				// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) {
		error("ERROR opening socket", 1);
	}
	/******************************
	** Enable the socket to begin listening & connect socket to port
	*******************************/
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR on binding", 1);
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	/******************************
	** Loop waiting for connections.
	** Process must be killed to break
	*******************************/
	while(1) {
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect, clientAddress will hold client data

		// Accept a connection, blocking if one is not available until one connects
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) {
			error("ERROR on accept", 1);
		}
		/******************************
		** Fork once handshake begins, child process will handle connection
		*******************************/
		spawnPid = fork();

		switch(spawnPid) {
			case -1:
				error("Hull Breach", 1);
				break;

			/******************************
			** Child: handles connection from otp_enc
			*******************************/
			case 0:
				memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
				charsRead = recvMSG(establishedConnectionFD, buffer, CHARMAX, 0);	// Receive confirmation from client

				if (strcmp(buffer, encStr) != 0) {	// If process is not otp_enc
					charsWritten = sendMSG(establishedConnectionFD, "error", 0);
					close(establishedConnectionFD);		// close and exit process
					exit(0);
					break;
				}
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0); 	// Send confirmation to otp_enc
				/*****************************
				** Receive key size from otp_enc
				******************************/
				memset(buffer, '\0', CHARMAX); 				// Clear out the buffer again for reuse
				charsRead = recvMSG(establishedConnectionFD, buffer, CHARMAX, 0);	// Receive key from otp_enc
				buffer[strcspn(buffer, "\n")] = '\0';	// Remove newline from buffer
				keySize = atoi(buffer);								// Convert string to int
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0);	// Send text to otp_enc_d
				/*****************************
				** Receive key from otp_enc
				******************************/
				memset(keyBuf, '\0', CHARMAX); 				// Clear out the buffer again for reuse
				charsRead = recvMsgChar(establishedConnectionFD, keyBuf, CHARMAX, keySize, 0);	// Receive key from otp_enc
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0);	// Send text to otp_enc_d
				/*****************************
				** Receive text size from otp_enc
				******************************/
				memset(buffer, '\0', CHARMAX); 				// Clear out the buffer again for reuse
				charsRead = recvMSG(establishedConnectionFD, buffer, CHARMAX, 0);	// Receive key from otp_enc
				buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline from buffer
				textSize = atoi(buffer);							// Convert string to int
				charsWritten = sendMSG(establishedConnectionFD, encStr, 0);
				/*****************************
				** Receive text from otp_enc
				******************************/
				memset(textBuf, '\0', CHARMAX);				// Clear out the textBuf for reuse
				charsRead = recvMsgChar(establishedConnectionFD, textBuf, CHARMAX, textSize, 0); // Receive plain text from otp_enc
				/*****************************
				** Encrypt Ciphertext & send to otp_enc
				******************************/
				encryptBuf(keyBuf, textBuf, cipher);
				charsWritten = sendMSG(establishedConnectionFD, cipher, 0); 	// Send ciphertext back

				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				exit(0);
				break;

			/******************************
			** Parent Process: check child
			*******************************/
			default:
				spawnPid = waitpid(-1, &spChildExit, WNOHANG);
				break;
		}
		// flush stdout if child exits
		while ((spawnPid = waitpid(-1, &spChildExit, WNOHANG)) > 0) {
			fflush(stdout);
		}
	}
	close(listenSocketFD); // Close the listening socket
	return 0;
}
