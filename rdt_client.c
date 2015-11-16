/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Client definition file
*/

#include "config.h"
#include "client_lib.h"

// [CLIENT VARIABLES]
int client_sockfd;			// Socket file descriptor for communicating to remote host
int isConnected = 0;		// Connected to server = 1, 0 otherwise
pthread_t waitDataThread;	// Thread to receive data

// Receive data
/*void *waitData(void *param) {

	// [ VARIABLES FOR RECEIVING PACKETS ]
	int recvd;					// Number of bytes read into the buffer
	struct packet msgrecvd;		// Data packet received

	// [ VARIABLES FOR RECEIVING FILES ]
	int isReceiving = 0;				// File flag: 1=receiving files, 0 otherwise
	char fileRecvName[FILENAMESIZE];	// Filename of file received
	FILE *fp;							// File pointer of file received
	int counter = 0;					// Counter to prepend if filename is a duplicate

	// Set isconnected flag since client successfully connected to server
	isConnected = 1;

	// Send confirm to server
	struct packet toSend;
	if(createPacket("CONFIRM", &toSend) == -1) {
		fprintf(stderr, "Can't create data to send. Try again.\n");
	}
	else { int sent = sendDataToServer(&toSend); }
	memset(&toSend, 0, sizeof toSend);	// Empty the struct

	// While connection with server is alive
	while(isConnected) {

		// recv() data from the socket
		recvd = recv(client_sockfd, (void *)&msgrecvd, sizeof(struct packet), 0);
		
		// If recv() returns 0, server has closed the connection
		if (!recvd) {
			fprintf(stderr, "Server connection lost. \n");
			isConnected = 0; close(client_sockfd); break;
		}

		if (recvd > 0) {

			// Special case: when receiving files
			if (strcmp(msgrecvd.command, "FILE_SEND") == 0) {

				// First file packet received
				if (isReceiving == 0) {

					// If file already exists in current directory, prepend a number to filename
					if (access(msgrecvd.filename, F_OK) != -1) {
						char prepend[MAXCOMMANDSIZE];
						sprintf(prepend, "%i", counter);
						strncpy(fileRecvName, prepend, MAXCOMMANDSIZE);
						strcat(fileRecvName,msgrecvd.filename);
						counter++;
					}
					else {
						strncpy(fileRecvName, msgrecvd.filename, MAXCOMMANDSIZE);
					}					

					fprintf(stdout, "Downloading %s...\n", fileRecvName);

					// Open for writing
					fp = fopen(fileRecvName, "ab");
					if (fp == NULL) { fprintf(stdout, "File write error. Check your file name.\n"); }
					fwrite(msgrecvd.message, 1, msgrecvd.filebytesize, fp);

					// Change status to currently receiving file data
					isReceiving = 1;
				}
				// Rest of the file packets
				else { fwrite(msgrecvd.message, 1, msgrecvd.filebytesize, fp); }
			}
			// End of file
			else if (strcmp(msgrecvd.command, "FILE_END") == 0) {
				fclose(fp); isReceiving = 0;
				fprintf(stdout, "%s saved.\n", fileRecvName);
			}
			// Other cases
			else { receivedDataHandler(&msgrecvd); }
		}

		// Make sure the struct is empty
		memset(&msgrecvd, 0, sizeof(struct packet));
	}
}*/

// Parse client commands
void parseCommand(char * command) {

	const char *delimiter = " ";

	// Split the command and entry
	char *token = strsep(&command, delimiter);
	char *params[2] = {0};
	params[0] = token;
	params[1] = command;

	if (strcmp(params[0], CONNECT) == 0) {
		client_sockfd = connectToServer();
		//pthread_create(&waitDataThread, NULL, waitData, NULL);
		if (client_sockfd == -1) {
			return;
		}
	} 
	else if (strcmp(params[0], CHAT) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [alias]\n", CHAT);
			return;
		}
		createMessage(client_sockfd, params[0], params[1]);
	} 
	else if (strcmp(params[0], QUIT) == 0) {
		createMessage(client_sockfd, params[0], params[1]);
	}
	else if (strcmp(params[0], TRANSFER) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [filename]\n", TRANSFER);
			return;
		}
		createMessage(client_sockfd, params[0], params[1]);
	} 
	else if (strcmp(params[0], HELP) == 0) {
		help();
	}
	else if (strcmp(params[0], MESSAGE) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [message]\n", MESSAGE);
			return;
		}
		createMessage(client_sockfd, params[0], params[1]);
	}
	else if (strcmp(params[0], EXIT) == 0) {
		printf("Closing the chat client...\n");
		isConnected = 0;
		close(client_sockfd);
		printf("Any active sockets closed.\n");
		exit(1);
	}
	else {
		printf("Invalid command: %s. \nType '%s' for command list.\n", params[0], HELP);
	}
}

// Main loop for the client
int main(int argc, char *argv[]) {

	// Check user arguments supplied
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [gbn|sr] \n-- Where: gbn = Go-back-N, sr = Selective-repeat\n", argv[0]);
		exit(1);
	}
	// Check selected protocol and initialize datalink layer
	else if (strcmp(argv[1], "gbn") == 0 || strcmp(argv[1], "sr") == 0) {

    	printf("Protocol: %s\n", argv[1] );
    	
    	// Initialize datalink layer
    	//datalink_init(argv[1]);
    }
    // Unrecognized transfer protocol
    else {
    	fprintf(stderr, "Error: Unrecognized transfer protocol. \n");
    	exit(1);
    }

    // For user command entries
    char user_command[USERCOMMAND];

    // Client command loop
    while (1) {

    	memset(user_command, 0, USERCOMMAND);	// always clear out user_command
		printf("CLIENT> "); 					// prompt user for command
		
		if (fgets(user_command, USERCOMMAND, stdin) == NULL) {
			continue;
		}

		// Terminate string command
		user_command[strlen(user_command) - 1] = '\0';
		
		// Valid command format: -[command]
		if (user_command[0] == '-') {
			parseCommand(user_command);
			continue;
		}
		else {
			if (strcmp(removeSpace(user_command), "") == 0) {
				continue;
			}
            printf("Invalid command: %s. \nType '%s' for command list.\n", user_command, HELP);
			continue;
		}
	}

	return 0;
}
