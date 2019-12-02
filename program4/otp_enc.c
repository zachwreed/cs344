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
// Boolean
#define TRUE 1
#define FALSE 0

// Error function used for reporting issues
// change to error(msg, exit status) later -------------------------------------
void error(const char *msg) {
	perror(msg);
	exit(0);
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

	FILE *keyF = NULL; 	// holds stdin key file
	FILE *textF = NULL; // holds stdin text fileIn


	struct sockaddr_in serverAddress;		// Holds address of otp_enc_d process plus settings
	struct hostent* serverHostInfo;			// Holds hostname from args
	char buffer[256];										// char array used for receiving from otp_enc_d

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
				error("input contains bad characters");
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
			error(argv[isRdIdx]);
		}
		// Redirect stdout to outFile
		outDup = dup2(outFile, 0);
		if (outDup < 0) {
			error("open error");
		}
	}

	// Open Stdin File
	textF = fopen(argv[1], "r");
	memset(buffer, '\0', sizeof(buffer)); 		// Clear out the buffer array
	fgets(buffer, sizeof(buffer) - 1, textF); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	buffer[strcspn(buffer, "\n")] = '\0'; 		// Remove the trailing \n that fgets adds


	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); 									// Get the port number, convert to an integer
	serverHostInfo = gethostbyname("localhost"); 	// Convert the machine name into a type of address
	serverAddress.sin_family = AF_INET; 					// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber);		// Store the port number

	if (serverHostInfo == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(0);

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
	if (socketFD < 0) error("CLIENT: ERROR opening socket");

	/***************************
	**  Connect socket to server address
	** int <0 success, -1 failure> connect(int sockedFD, cast otp_enc_d &address, size_t otp_enc_d address)
	** domain = AF_INET for cross-network, AF_UNIX for same-machine
	** type = UPD or TCP
	****************************/
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		error("CLIENT: ERROR connecting");
	}

	/***************************
	** Send Data to otp_enc_d process
	** size_t <bytes sent> send(int sockedFD, message, message size_t, int flags)
	****************************/
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); 	// Write to the server
	if (charsWritten < 0) {
		error("CLIENT: ERROR writing to socket");
	}

	if (charsWritten < strlen(buffer)) {
		printf("CLIENT: WARNING: Not all data written to socket!\n");
	}

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse

	/***************************
	** Receive Data to otp_enc_d process
	** size_t <bytes received> recv(int sockedFD, message, message size_t, int flags)
	****************************/
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data leaving \0 at end
	if (charsRead < 0) {
		error("CLIENT: ERROR reading from socket");
	}

	printf("CLIENT: I received this from the server: \"%s\"\n", buffer);

	close(socketFD);	// Close the socket
	// Close Stdout File if Redirect
	if (isRd == TRUE) {
		close(outFile);
	}
	return 0;
}
