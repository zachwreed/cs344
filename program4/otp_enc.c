#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
// Boolean
#define TRUE 1
#define FALSE 0
#define CHARMAX 70000
#define CHARBYTE 10

// Error function used for reporting issues
// change to error(msg, exit status) later -------------------------------------
void error(const char *msg, int status) {
	perror(msg);
	exit(status);
}

int recvMSG(int connectionFD, char* buffer, int bSize, int flag) {
	int charsRead, charsTotal = 0;

	while(!strcspn(buffer, "\n")) {
		charsRead = recv(connectionFD, buffer, bSize, flag);
		if (charsRead <= 0) {
			error("ERROR reading from socket", 1);
			break;
		}
		charsTotal += charsRead;
	}
	return charsTotal;
}

int sendMSG(int connectionFD, char* buffer, int flag) {
	int charsWritten, charsTotal = 0;
	// block until all data is sent
	printf("CLIENT: size of text: %lu\n", strlen(buffer));
	while (charsTotal < strlen(buffer)) {
		charsWritten = send(connectionFD, buffer, strlen(buffer), flag);
		if (charsWritten < 0) {
			error("CLIENT: ERROR writing to socket", 1);
		}
		if (charsWritten == 0) {
			break;
		}
		charsTotal += charsWritten;
	}
	printf("CLIENT: charsWritten: %d\n", charsTotal);
	return charsTotal;
}

int main(int argc, char *argv[]) {
	int socketFD;				// socket file descriptor
	int portNumber;			// port of otp_enc_d process
	int charsWritten;		// char[] size sent to otp_enc_d process
	int charsRead;			// char[] size received from otp_enc_d response
	int argValid;				// return from check_args

	int isBG = FALSE;		// is background
	int isRd = FALSE;		// is file redirect
	int isRdIdx = -1;		// index of argv[] where file will be redirected
	int outFile = -1; 	// holds stdout redirect file
	int outDup = -1;		// holds dup2 return

	char* encStr = "otp_enc\n";

	char* encdStr = "otp_enc_d\n";
	// strcat(encStr, "\n");
	// strcat(encdStr, "\n");


	// printf("CLIENT Declaration: %s\n", encStr);
	// printf("CLIENT Declaration: %s\n", encdStr);

	FILE *keyF = NULL; 				// holds stdin key file
	FILE *textF = NULL; 			// holds stdin text fileIn
	char keyBuf[CHARMAX];			// holds buffer for keyfile
	char textBuf[CHARMAX];		// holds buffer for textfile
	char ktBuf[CHARBYTE];			// holds buffer for keyfile + textfile data
	char buffer[CHARMAX];			// char array used for recv otp_enc_d
	long ktSize;							// int to hold size of key and text combined

	struct sockaddr_in serverAddress;		// Holds address of otp_enc_d process plus settings
	struct hostent* serverHostInfo;			// Holds hostname from args

	/******************************
	** Check Args
	*******************************/
	if (argc < 4 || argc > 7) {
		fprintf(stderr,"input contains bad characters\n");
		exit(0);
	}

	// Loop and check arguments for redirection
	int i;
	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], ">") == 0) {
			// if redirection is in range
			if (i != (argc - 1) && i != 0) {
				error("input contains bad characters", 1);
			}
			else {
				isRd = TRUE;
				isRdIdx = i + 1;
			}
		}
	}

	// If stdout redirection
	if (isRd == TRUE) {
		// Open outFile from argv
		outFile = open(argv[isRdIdx], O_WRONLY | O_TRUNC | O_CREAT, 0644);
		if (outFile < 0) {
			error(argv[isRdIdx], 1);
		}
		// Redirect stdout to outFile
		outDup = dup2(outFile, 0);
		if (outDup < 0) {
			error("open error", 1);
		}
	}
	// Open Plain Text Stdin Files
	textF = fopen(argv[1], "r");
	memset(textBuf, '\0', sizeof(textBuf)); 		// Clear out the buffer array
	fgets(textBuf, sizeof(textBuf) - 1, textF); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	//textBuf[strcspn(textBuf, "\n")] = '\0'; 		// Remove the trailing \n that fgets adds

	// Open Key Stdin File
	keyF = fopen(argv[2], "r");
	memset(keyBuf, '\0', sizeof(keyBuf)); 		// Clear out the buffer array
	fgets(keyBuf, sizeof(keyBuf) - 1, keyF); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	//keyBuf[strcspn(keyBuf, "\n")] = '\0'; 		// Remove the trailing \n that fgets adds

	// printf("key char length: %lu\n", strlen(keyBuf));
	// printf("tex char length: %lu\n", strlen(textBuf));

	if (strlen(keyBuf) < (strlen(textBuf) + 1)) {
		error("Error: key is too short", 1);
	}
	// Get size of key + text file
	memset(ktBuf, '\0', sizeof(ktBuf));					// Clear out the buffer array
	ktSize = strlen(keyBuf) + strlen(textBuf);	// Get size of key & text buffer
	sprintf(ktBuf, "%ld", ktSize);							// Copy long value to key-text buffer
	ktBuf[ktSize - 1] = '\n';										// Set Last Char to newline for server to catch

	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); 									// Get the port number, convert to an integer
	serverHostInfo = gethostbyname("localhost"); 	// Convert the machine name into a type of address
	serverAddress.sin_family = AF_INET; 					// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber);		// Store the port number

	if (serverHostInfo == NULL) {
		error("otp_enc error: could not resolve serverHostInfo", 1);
	}
	 // Copy in the address memcpy(destination, source, size_t)
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	/***************************
	**  Set up the socket
	** int <file descriptor> = socket(int domain, int type, in protocol)
	** domain = AF_INET for cross-network, AF_UNIX for same-machine
 	** type = UPD or TCP
	****************************/
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) {
		error("opt_enc error: could not create socket", 1);
	}
	/***************************
	**  Connect socket to server address
	** int <0 success, -1 failure> connect(int sockedFD, cast otp_enc_d &address, size_t otp_enc_d address)
	** domain = AF_INET for cross-network, AF_UNIX for same-machine
	** type = UPD or TCP
	****************************/
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		error("otp_enc error: could not contact otp_enc on port", 1);
	}

	/***************************
	** Send & Receive Data to & from otp_enc_d process
	****************************/

	charsWritten = sendMSG(socketFD, encStr, 0);				// Send confirmation process to daemon
	memset(buffer, '\0', sizeof(buffer)); 							// Clear out the buffer again for reuse
	charsRead = recvMSG(socketFD, buffer, CHARMAX, 0);	// Receive confirmation process from daemon
	//buffer[strcspn(buffer, "\n")] = '\0';
	// printf("CLIENT: recv: %s", buffer);
	// printf("CLIENT: encd: %s", encStr);
	if (strcmp(buffer, encStr) != 0) {							// If process is otp_enc_d & not otp_dec_d
		error("Cannot access otp_enc_d on port", 1);
		return 1;
	}
	charsWritten = sendMSG(socketFD, keyBuf, 0);				// Send key to otp_enc_d
	memset(buffer, '\0', sizeof(buffer)); 							// Clear out the buffer again for reuse
	charsRead = recvMSG(socketFD, buffer, CHARMAX, 0);	// Receive confirmation from otp_enc_d
	//buffer[strcspn(buffer, "\n")] = '\0';
	if (strcmp(buffer, encStr) != 0) {									// Check confirmation
		error("Cannot access otp_enc_d on port", 1);
		return 1;
	}
	charsWritten = sendMSG(socketFD, textBuf, 0);				// SEnd text to otp_enc_d
	memset(buffer, '\0', sizeof(buffer)); 							// Clear out the buffer again for reuse
	charsRead = recvMSG(socketFD, buffer, CHARMAX, 0); 	// Receive ciphertext from otp_enc_d

	printf("%s\n", buffer);

	close(socketFD);	// Close the socket
	// Close Stdout File if Redirect
	if (isRd == TRUE) {
		close(outFile);
	}
	return 0;
}
