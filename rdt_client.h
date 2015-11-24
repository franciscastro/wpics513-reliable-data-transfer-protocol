/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Client header file.
*/

#ifndef RDT_CLIENT_H_
#define RDT_CLIENT_H_

#include "config.h"

int client_sockfd;		// Socket file descriptor for communicating to remote host
int isconnected = 0;	// 1 if already connected to server, 0, -1, -2, otherwise (see connectToHost())

char alias[MAXCOMMANDSIZE];		// This client's alias

typedef struct ThreadData {
	pthread_t thread_ID;	// This thread's pointer
} ThreadData;

char* removeSpace(char *s);
void parseCommand(char * command);
void *get_in_addr(struct sockaddr *sa);
int fetchServerHostname(char *hostname);
int commandTranslate(char *command);
int connectToHost(struct addrinfo *hints, struct addrinfo **servinfo, int *error_status, char *hostname, struct addrinfo **p);
void *receiver(void *param);
void receivedDataHandler(struct packet *msgrecvd);
off_t filesize(const char *filename);
int sendDataToServer(struct packet *packet);
int sendFilePackets();
int createPacket(const char *command, struct packet *toSend);
void allCaps(char *command);

//=================================================================================

// Remove whitespace
char* removeSpace(char *s) {

    size_t size = strlen(s);	// Check for valid length
    if (!size) { return s; }

    char *end = s + size - 1;
    while (end >= s && isspace(*end)) { end--; }

    *(end + 1) = '\0';	// Terminate this string

    while (*s && isspace(*s)) { s++; }
    
    return s;
}

//=================================================================================

// Convert strings to all uppercase
void allCaps(char *command) {
	while(*command != '\0') {
		*command = toupper(*command);
		command++;
	}
}

//=================================================================================

// Parse client commands
void parseCommand(char * command) {

	const char *delimiter = " ";

	// Split the command and entry
	char *token = strsep( &command, delimiter );
	char *params[2] = {0};
	params[0] = token;
	params[1] = command;

	// CONNECT
	if ( strcmp(params[0], CONNECT) == 0 ) {
		client_sockfd = connectToServer();
		//pthread_create(&waitDataThread, NULL, waitData, NULL);
		if (client_sockfd == -1) {
			return;
		}
	} 
	// CHAT
	else if ( strcmp(params[0], CHAT) == 0 ) {
		if ( params[1] == NULL ) {
			printf( "Usage: %s [alias]\n", CHAT );
			return;
		}
		createMessage( client_sockfd, params[0], params[1] );
	}
	// QUIT 
	else if ( strcmp(params[0], QUIT) == 0 ) {
		createMessage( client_sockfd, params[0], params[1] );
	}
	// TRANSFER
	else if ( strcmp(params[0], TRANSFER ) == 0) {
		if ( params[1] == NULL ) {
			printf( "Usage: %s [filename]\n", TRANSFER );
			return;
		}
		createMessage( client_sockfd, params[0], params[1] );
	}
	// HELP
	else if ( strcmp(params[0], HELP) == 0) {
		help();
	}
	// MESSAGE
	else if ( strcmp(params[0], MESSAGE) == 0 ) {
		if ( params[1] == NULL ) {
			printf( "Usage: %s [message]\n", MESSAGE );
			return;
		}
		createMessage( client_sockfd, params[0], params[1] );
	}
	// EXIT
	else if ( strcmp(params[0], EXIT ) == 0) {
		printf( "Closing the chat client...\n" );
		isConnected = 0;
		close( client_sockfd );
		printf( "Active sockets closed.\n" );
		exit(1);
	}
	// CONFIRM
	else if ( strcmp(params[0], CONFIRM) == 0 ) {
		createMessage( client_sockfd, params[0], params[1] );
	}
	// INVALID INPUT
	else {
		printf( "Invalid command: %s. \nType '%s' for command list.\n", params[0], HELP );
	}
}

//=================================================================================

// Get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {

	// sockaddr is IPv4
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	// else, sockaddr is IPv6
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//=================================================================================

// Get server hostname from file HOSTNAME
// Returns 1 on success, 0 on error
int fetchServerHostname(char *hostname) {

	char hostfile[] = "HOSTNAME";

	// Create file pointer
	FILE *fp = fopen(hostfile, "rb");
	
	// If file does not exist
	if (fp == NULL) {
		fprintf(stdout, "File open error. Make sure HOSTNAME file exists.\n\n");
		return 0;
	}

	while (!feof(fp)) {
		fread(hostname, 1, filesize(hostfile), fp);
	}

	// Manual removal of newline character
	int len = strlen(hostname);
	if (len > 0 && hostname[len-1] == '\n') {
		hostname[len-1] = '\0';
	}

	fclose(fp);
	return 1;
}

//=================================================================================

// Translates user's command into this program's integer representation
int commandTranslate(char *command) {

	if (strcmp(command,"CONNECT") == 0) { return 1; }
	else if (strcmp(command, "CHAT") == 0) { return 2; }
	else if (strcmp(command,"QUIT") == 0) { return 3; }
	else if (strcmp(command, "TRANSFER") == 0) { return 4; }
	else if (strcmp(command, "FLAG") == 0) { return 5; }
	else if (strcmp(command, "HELP") == 0) { return 6; }
	else if (strcmp(command, "MESSAGE") == 0) { return 7; }
	else if (strcmp(command, "EXIT") == 0) { return 8; }
	else if (strcmp(command, "CONFIRM") == 0) { return 9; }
	else { return -1; }

}

//=================================================================================

// Connect to TCR server
// Returns: -1 on getaddrinfo() fail
//			-2 on fail to connect a socket to remote host
//			-3 on fail to create thread for recv()ing data
//			 1 on successful connect
int connectToHost(struct addrinfo *hints, struct addrinfo **servinfo, int *error_status, char *hostname, struct addrinfo **p) {

	// [ Load up address structs with getaddrinfo() ]
	//=================================================================================

	// Setup values in hints
	memset(hints, 0, sizeof *hints);	// make sure the struct is empty
	(*hints).ai_family = AF_UNSPEC;		// don't care if IPv4 (AF_INET) or IPv6 (AF_INET6)
	(*hints).ai_socktype = SOCK_STREAM;	// use TCP stream sockets
	
	// Call getaddrinfo() to setup the structures in hints
	// - error: getaddrinfo() returns non-zero
	// - success: *servinfo will point to a linked list of struct addrinfo,
	//			  each of which contains a struct sockaddr to be used later
	if (((*error_status) = getaddrinfo(hostname, PORT, hints, &(*servinfo))) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror((*error_status)));
		return -1;
	}

	//=================================================================================


	// [ Make a socket, connect() to destination ]
	//=================================================================================

	// Loop through all the results in *servinfo and bind to the first we can
	for((*p) = (*servinfo); (*p) != NULL; (*p) = (*p)->ai_next) {
		
		// Make a socket
		// - assign a socket descriptor to client_sockfd on success, -1 on error
		if ((client_sockfd = socket((*p)->ai_family, (*p)->ai_socktype, (*p)->ai_protocol)) == -1) 
		{
			perror("Client socket");
			continue;
		}

		// Connect to a remote host in the destination port and IP address
		// - returns -1 on error and sets errno to the error's value
		if (connect(client_sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) 
		{
			close(client_sockfd);
			perror("Client connect");
			continue;
		}

		break;
	}

	// Free the linked list when all done with *servinfo
	freeaddrinfo(*servinfo);

	// If *servinfo is empty, then fail to connect
	if ((*p) == NULL) 
	{
		fprintf(stderr, "Client: Failed to connect\n");
		return -2;
	}

	// Create pthread for recv()ing data from the socket
	struct threaddata newthread;
	if(pthread_create(&newthread.thread_ID, NULL, receiver, (void *)&newthread)) 
	{
		fprintf(stderr, "Error creating recv() thread. Try connecting again.\n");
		close(client_sockfd);
		return -3;
	}

	// Prompt client that it is now waiting to recv() on pthread
	fprintf(stdout, "Now waiting for data on [%i]...\n", client_sockfd);

	// Set isconnected flag since client successfully connected to server
	isconnected = 1;

	return 1;

	//=================================================================================

}

//=================================================================================

// For recv()ing messages from the socket
void *receiver(void *param) {
	
	int recvd;
	struct packet msgrecvd;
	FILE *fp;
	int isReceiving = 0;	// 0 - not receiving files, 1 - receiving files
	char fileRecvName[MAXCOMMANDSIZE];
	int counter = 0;

	// Set isconnected flag since client successfully connected to server
	isconnected = 1;

	// Send confirm to server
	struct packet toSend;
	if(createPacket("CONFIRM", &toSend) == -1) {
		fprintf(stderr, "Can't create data to send. Try again.\n");
	}
	else {
		int sent = sendDataToServer(&toSend);
	}
	memset(&toSend, 0, sizeof(struct packet));	// Empty the struct

	// While connection with server is alive
	while(isconnected) {

		// recv() data from the socket
		recvd = recv(client_sockfd, (void *)&msgrecvd, sizeof(struct packet), 0);
		
		if (!recvd) {
			fprintf(stderr, "Server connection lost. \n");
			isconnected = 0;
			close(client_sockfd);
			break;
		}

		if (recvd > 0) {

			//fprintf(stdout, "COMMAND: %s\nMESSAGE: %s\n", msgrecvd.command, msgrecvd.message);
			fflush(stdout);

			// Special case: when receiving files
			if (strcmp(msgrecvd.command, "FILE_SEND") == 0) {

				// First file packet received
				if (isReceiving == 0) {

					// If file already exists in current directory, prepend a number to filename
					if (access(msgrecvd.filename, F_OK) != -1) {
						char prepend[MAXCOMMANDSIZE];
						sprintf(prepend, "%i", counter);
						//fprintf(stdout, "in here: %s\n", prepend);
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
				else {
					//fprintf(stdout, ">>>>> REST OF THE FILE\n");
					fwrite(msgrecvd.message, 1, msgrecvd.filebytesize, fp);
				}

			}
			// End of file
			else if (strcmp(msgrecvd.command, "FILE_END") == 0) {
				//fprintf(stdout, ">>>>> FCLOSE PART\n");
				fclose(fp);
				isReceiving = 0;
				fprintf(stdout, "%s saved.\n", fileRecvName);
			}
			// Other cases
			else {
				//fprintf(stdout, ">>>>> OTHER CASES\n");
				receivedDataHandler(&msgrecvd);
			}

			//fprintf(stdout, "%s\n", msgrecvd.message);
		}

		// Make sure the struct is empty
		memset(&msgrecvd, 0, sizeof(struct packet));
	}
}

//=================================================================================

// Handling recv()ed messages from the socket
void receivedDataHandler(struct packet *msgrecvd) {

	if (strcmp((*msgrecvd).command, "ACKN") == 0) {
		//fprintf(stdout, "You are added in the chat queue\n");
		fprintf(stdout, "%s\n", (*msgrecvd).message);
	}
	else if (strcmp((*msgrecvd).command, "IN SESSION") == 0) {
		// fprintf(stdout, "Now on a channel with: %s\n", (*msgrecvd).message);
		fprintf(stdout, "%s\n", (*msgrecvd).message);
	}
	else if (strcmp((*msgrecvd).command, "QUIT") == 0) {
		//fprintf(stdout, "You have quit the channel \n");
		fprintf(stdout, "%s\n", (*msgrecvd).message);
	}
	else if (strcmp((*msgrecvd).command, "HELP") == 0) {
		//fprintf(stdout, "Commands available:\n %s\n", (*msgrecvd).message );
		fprintf(stdout, "%s\n", (*msgrecvd).message);
	}
	else if (strcmp((*msgrecvd).command, "MESSAGE") == 0) {
		fprintf(stdout, "[ %s ]: %s\n", (*msgrecvd).alias, (*msgrecvd).message );
	}
	else if (strcmp((*msgrecvd).command, "CHAT_ACK") == 0) {
		fprintf(stdout, "Now chatting with %s\n", (*msgrecvd).alias);
	}
	else if (strcmp((*msgrecvd).command, "NOTIF") == 0) {
		fprintf(stdout, "%s\n", (*msgrecvd).message);
	}

}

//=================================================================================

// Determing file size, returns -1 on error
off_t filesize(const char *filename) {
	struct stat st;

	// File size can be determined
	if (stat(filename, &st) == 0) {
		return st.st_size;
	}

	// File size cannot be determined
	fprintf(stderr, "Cannot determine size of %s: %s\n", filename, strerror(errno));
	return -1;

}

//=================================================================================

// Send a message to TCR server
int sendDataToServer(struct packet *packet) {

	int packetlen = sizeof *packet;
	int total = 0;				// How many bytes we've sent
    int bytesleft = packetlen;	// How many we have left to send
    int n;

    // To make sure all data is sent
    while(total < packetlen) {

        n = send(client_sockfd, (packet + total), bytesleft, 0);
        
        if (n == -1) { break; }
        
        total += n;
        bytesleft -= n;

    }

    packetlen = total;	// Return number actually sent here

    memset(packet, 0, sizeof(struct packet));	// Empty the struct
	return n == -1 ? -1 : 0;	// Return 0 on success, -1 on failure
}

//=================================================================================

// Send a file to the server, returns 0 on success
int sendFilePackets() {

	// Get file name from user
	char filename[50];
	fprintf(stdout, "File to send: ");
	fgets(filename, sizeof filename, stdin);
	
	// Manual removal of newline character
	int len = strlen(filename);
	if (len > 0 && filename[len-1] == '\n') { filename[len-1] = '\0'; }

	// If file is > 100 MB, don't send
	int fullsize = filesize(filename);
	if (fullsize > 104857600) {
		fprintf(stdout, "Sending files > 100 MB is prohibited.\n");
		return 1;
	}
	else {
		fprintf(stdout, "File size: %i bytes\n", fullsize);
	}
	
	// Create file pointer
	FILE *fp = fopen(filename, "rb");
	
	// If file does not exist
	if (fp == NULL) {
		fprintf(stdout, "File open error. Check your file name.\n");
		return 1;
	}

	// Read and send file packets
	while(!feof(fp)){

		// file buffer to store chunks of files
		char filebuff[MAXMESSAGESIZE];

		// Outbound data packet
		struct packet outbound;
		strncpy(outbound.command, "TRANSFER", MAXCOMMANDSIZE);
		strncpy(outbound.filename, filename, MAXCOMMANDSIZE);

		// Read data from file
		int numread = fread(outbound.message, 1, MAXMESSAGESIZE, fp);
		//fprintf(stdout, "Bytes read: %i\n", numread);
		//strncpy(outbound.message, filebuff, MAXMESSAGESIZE);
		outbound.filebytesize = numread;

		strncpy(outbound.alias, alias, MAXCOMMANDSIZE);
		
		// Send data to server
		sendDataToServer(&outbound);

		memset(&outbound, 0, sizeof(struct packet));	// Empty the struct
	}

	fclose(fp);

	// File end packet
	struct packet outbound;
	strncpy(outbound.command, "TRANSFER_END", MAXCOMMANDSIZE);
	strncpy(outbound.filename, filename, MAXCOMMANDSIZE);
	sendDataToServer(&outbound);
	memset(&outbound, 0, sizeof(struct packet));	// Empty the struct

	return 0;

}

//=================================================================================

// Creates packet to be sent out, returns -1 on error
int createPacket(const char *command, struct packet *toSend) {

	struct packet outbound;

	// Default values to avoid garbage data
	strncpy(outbound.command, command, MAXCOMMANDSIZE);	// Put command type in packet
	strncpy(outbound.message, "message", MAXMESSAGESIZE);
	strncpy(outbound.alias, "alias", MAXCOMMANDSIZE);
	strncpy(outbound.filename, "filename", MAXCOMMANDSIZE);
	outbound.filebytesize = 0;

	if (strcmp(command,"CONNECT") == 0) {
		(*toSend) = outbound;
		return 1;
	}
	else if (strcmp(command, "CHAT") == 0) {	
		
		// For getting alias from user
		char useralias[MAXCOMMANDSIZE];
		fprintf(stdout, "Enter alias: ");
		fgets(useralias, sizeof useralias, stdin);

		// Manual removal of newline character
		int len = strlen(useralias);
		if (len > 0 && useralias[len-1] == '\n') { useralias[len-1] = '\0'; }

		// If invalid alias
		if(useralias == NULL || strcmp(useralias," ") == 0 || strcmp(useralias,"\n") == 0) {
			fprintf(stderr, "Invalid alias structure.\n");
			return -1;
		}

		// Put alias in packet
		strncpy(outbound.alias, useralias, MAXCOMMANDSIZE);

		// Put alias in global
		strncpy(alias, useralias, MAXCOMMANDSIZE);

		(*toSend) = outbound;
		return 1;
	}
	else if (strcmp(command,"QUIT") == 0) {
		strncpy(outbound.alias, alias, MAXCOMMANDSIZE);
		(*toSend) = outbound;
		return 1;
	}
	else if (strcmp(command, "FLAG") == 0) {
		strncpy(outbound.alias, alias, MAXCOMMANDSIZE);
		(*toSend) = outbound;
		return 1;
	}
	else if (strcmp(command, "HELP") == 0) {
		(*toSend) = outbound;
		return 1;
	}
	else if (strcmp(command, "MESSAGE") == 0) {

		fprintf(stdout, "[ You ]: ");
		fgets(outbound.message, sizeof outbound.message, stdin);

		// Manual removal of newline character
		int len = strlen(outbound.message);
		if (len > 0 && outbound.message[len-1] == '\n') { outbound.message[len-1] = '\0'; }

		//fprintf(stdout, "Your message: %s", outbound.message);
		strncpy(outbound.alias,alias,MAXCOMMANDSIZE);
		(*toSend) = outbound;
		return 1;
	}
	else if (strcmp(command, "CONFIRM") == 0) {
		(*toSend) = outbound;
		return 1;
	}

}

//=================================================================================

// Creates message to be sent out, returns -1 on error
int createMessage(int c_sockfd, const char* command, const char* message) {

	// Create packet
	Packet * msg = malloc(sizeof(msg));

	if (strcmp(command, CHAT) == 0) {
		msg->msgType = CHAT_M;
		strncpy(msg->data, message, strlen(message));
		datalinkSend( c_sockfd, msg );
		return 1;
	}
	else if (strcmp(command, QUIT) == 0) {
		msg->msgType = QUIT_M;
		datalinkSend(c_sockfd, msg);
		return 1;
	}
	else if (strcmp(command, TRANSFER) == 0) {
		msg->msgType = TRANSFER_M;
		//strncpy(msg->data, message, strlen(message));
		// Send message to datalink
		return 1;
	}
	else if (strcmp(command, MESSAGE) == 0) {
		msg->msgType = MESSAGE_M;
		strncpy(msg->data, message, strlen(message));
		datalinkSend(c_sockfd, msg);
		return 1;
	}
}

//=================================================================================

// Show client commands
void help() {
	printf("Client commands:\n");
	printf("\t%-10s connect to the server\n", CONNECT);
	printf("\t%-10s request for a chat partner\n", CHAT);
	printf("\t%-10s quit the current chat channel\n", QUIT);
	printf("\t%-10s send a file to chat partner\n", TRANSFER);
	printf("\t%-10s print this help information\n", HELP);
	printf("\t%-10s send a message to chat partner\n", MESSAGE);
	printf("\t%-10s terminate and exit the program\n", EXIT);
	printf("\t%-10s check with the server if you are in a chat queue\n", CONFIRM);
}

//=================================================================================

#endif