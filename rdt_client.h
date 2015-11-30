/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 28 Nov 2015

Client header file.
*/

#ifndef RDT_CLIENT_H_
#define RDT_CLIENT_H_

#include "config.h"
#include "gbn.h"
#define PORT "3490" 

int client_sockfd;				// Socket file descriptor for communicating to remote host
boolean isconnected = false;	// Flag to denote this client's connection state

char alias[ALIASSIZE];			// This client's alias

typedef struct ThreadData {
	pthread_t thread_ID;		// This thread's pointer
} ThreadData;

ThreadData newthread;			// receiver() thread
ThreadData recvthread;			// receiver() thread

char* removeSpace(char *s);
void allCaps(char *command);
off_t filesize(const char *filename);
void *get_in_addr(struct sockaddr *sa);
void parseCommand(char * command);
void createMessage(const char* command, const char* message);
void sendFilePackets(const char* filename);
void help();
void *receiver(void *param);

// TEMPORARY FOR TESTING
Packet *datalinkRecv();
void datalinkSend(int sfd, Packet p);

struct addrinfo hints, *servinfo, *p;
int * error_status;
char * hostname = "anto-Lenovo-Y50-70";

int is_gbn;
GBN_instance client_sndr_gbn;
GBN_instance client_rcvr_gbn;
GBNThreadData GBNThreadData_c;

Packet *datalinkRecv() {
	return NULL;
}

void datalinkSend(int sfd, Packet p) {
	return;
}

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

// Determining file size, returns -1 on error
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

// Get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {

	// sockaddr is IPv4
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	// else, sockaddr is IPv6
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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
	if ( strcmp(params[0], CONNECT ) == 0 ) {
		createMessage( params[0], params[1] );
	} 
	// CHAT
	else if ( strcmp(params[0], CHAT ) == 0 ) {
		if ( params[1] == NULL ) {
			printf( "Usage: %s [alias]\n", CHAT );
			return;
		}
		createMessage( params[0], params[1] );
	}
	// QUIT 
	/*else if ( strcmp(params[0], QUIT ) == 0 ) {
		createMessage( params[0], params[1] );
	}*/
	// TRANSFER
	else if ( strcmp(params[0], TRANSFER ) == 0) {
		if ( params[1] == NULL ) {
			printf( "Usage: %s [filename]\n", TRANSFER );
			return;
		}
		createMessage( params[0], params[1] );
	}
	// HELP
	else if ( strcmp(params[0], HELP ) == 0) {
		help();
	}
	// MESSAGE
	else if ( strcmp(params[0], MESSAGE ) == 0 ) {
		if ( params[1] == NULL ) {
			printf( "Usage: %s [message]\n", MESSAGE );
			return;
		}
		createMessage( params[0], params[1] );
	}
	// EXIT
	else if ( strcmp(params[0], EXIT ) == 0 ) {
		printf( "Closing the chat client...\n" );
		isconnected = false;
		close( client_sockfd );
		printf( "Active sockets closed.\n" );
		exit(1);
	}
	// CONFIRM
	else if ( strcmp(params[0], CONFIRM ) == 0 ) {
		createMessage( params[0], params[1] );
	}
	// INVALID INPUT
	else {
		printf( "Invalid command: %s. \nType '%s' for command list.\n", params[0], HELP );
	}
}

//=================================================================================


// Creates message to be sent out
void createMessage(const char* command, const char* message) {

	// Create packet
	// Packet * msg = malloc(sizeof(msg));
	// (*msg)->msgType = CONNECT_M;
	// strncpy(msg->data, message, strlen(message));
	Packet msg;

	if ( strcmp(command, CONNECT) == 0 ) {
		msg.msgType = CONNECT_M;
		//strncpy( msg.data, message, strlen(message) );
		// struct addrinfo *hints, struct addrinfo **servinfo, int *error_status, char *hostname, struct addrinfo **p
		connectToHost(&hints, &servinfo, &error_status, hostname, &p);
	}
	else if ( strcmp(command, CHAT) == 0 ) {
		printf("RECEIVED CHAT COMMAND\n");
		msg.msgType = CHAT_M;
		strncpy( msg.alias, message, strlen(message) );
		strncpy( alias, message, strlen(message) );		// Set this client's global alias

		if (is_gbn == 1) {
			msg.sockfd = client_sockfd;

			BufferEntry * pbe = malloc(sizeof(BufferEntry));
			pbe->pkt = msg;

			push_pkt_snd_buffer(0, client_sockfd, pbe);
		}
	}
	/*else if ( strcmp(command, QUIT ) == 0) {
		msg.msgType = QUIT_M;
		memset( alias, 0, sizeof(alias) );	// Null this client's global alias
	}*/
	else if ( strcmp(command, TRANSFER ) == 0) {
		sendFilePackets(message);
		return;
	}
	else if ( strcmp(command, MESSAGE) == 0 ) {
		msg.msgType = MESSAGE_M;
		strncpy( msg.data, message, strlen(message) );
	}
	else if ( strcmp(command, CONFIRM) == 0 ) {
		msg.msgType = CONFIRM_M;
	}

	datalinkSend( client_sockfd, msg );

	memset( &msg, 0, sizeof(Packet) );	// Empty the struct
}

//=================================================================================

// Chunk a file and send file packets to datalink
void sendFilePackets(const char* filename) {
	
	// Create file pointer
	FILE *fp = fopen(filename, "rb");
	
	// If file does not exist
	if ( fp == NULL ) {
		fprintf( stderr, "File open error. Check your file name.\n" );
		return;
	}

	// If file is > 100 MB, don't send
	int fullsize = filesize( filename );
	if ( fullsize > 104857600 ) {
		printf( "Sending files > 100 MB is prohibited.\n" );
		return;
	}
	else {
		printf( "File size: %i bytes\n", fullsize );
	}	

	// Read and send file packets
	while( !feof(fp) ){

		// file buffer to store chunks of files
		char filebuff[MAXDATA];

		// Create packet
		Packet msg;
		msg.msgType = TRANSFER_M;
		strncpy( msg.filename, filename, strlen(filename) );

		// Read data from file
		int numread = fread( msg.data, 1, MAXDATA, fp );
		printf( "Bytes read: %i\n", numread );
		
		// Send data to datalink
		datalinkSend( client_sockfd, msg );

		memset( &msg, 0, sizeof(Packet) );	// Empty the struct
	}

	// Close the file pointer
	fclose(fp);

	// File end packet
	Packet msg;
	msg.msgType = TRANSFER_END_M;
	strncpy( msg.filename, filename, strlen(filename) );
	
	// Send data to datalink
	datalinkSend( client_sockfd, msg );
	
	memset( &msg, 0, sizeof(Packet) );	// Empty the struct
}

//=================================================================================

// Show client commands
void help() {
	printf("Client commands:\n");
	printf("\t%-10s connect to the server\n", CONNECT);
	printf("\t%-10s request for to chat with server\n", CHAT);
	//printf("\t%-10s quit the current chat channel\n", QUIT);
	printf("\t%-10s send a file to chat partner\n", TRANSFER);
	printf("\t%-10s print this help information\n", HELP);
	printf("\t%-10s send a message to chat partner\n", MESSAGE);
	printf("\t%-10s terminate and exit the program\n", EXIT);
	printf("\t%-10s check with the server if you are in a chat queue\n", CONFIRM);
}

//=================================================================================

void* physical_receiver(void* param) {
	while(1) {
		Frame msgrecvd;
		int nbytes = recv(client_sockfd, (void *)&msgrecvd, sizeof(Frame), 0);

		if (nbytes <= 0) {
			if (nbytes == 0)
	            printf("CLIENT: server %d hung up\n", client_sockfd);
	        else 
	            perror("recv");
	        // simple socket close
	        printf("Closing socket\n");	
	        close(client_sockfd);
	        isconnected = 0;
	        break;
		}
	    else {
	    	// printf("RECV SOMETHING\n");
	    	if (msgrecvd.type != ACK_F) {
				printf("CLIENT: receiving frame \n");
				printf("CLIENT: pkt msgType %d\n", msgrecvd.pkt.msgType);
				printf("CLIENT: seqNumber %d\n", msgrecvd.seqNumber);
				printf("CLIENT: checksum is %s\n", msgrecvd.checksum);

				printf("Received command: %d from %d\n", msgrecvd.pkt.msgType, client_sockfd);
			}
			else {
				// printf("CLIENT: receiving ACK frame \n");
				// printf("CLIENT: seqNumber %d\n", msgrecvd.seqNumber);
			}

			gbn_rdt_rcv(&client_rcvr_gbn, msgrecvd, client_sockfd); // must handle ACKS properly!
	    }
	}
}

// For recv()ing messages from the socket
void *receiver(void *param) {

	printf("Initializing client receiver\n");

	// File variables
	FILE *fp;						// File pointer
	boolean isReceiving = false;	// Flag if client is currently receiving a file
	char *fileRecvName;				// File name of file currently being received
	int counter = 0;				// Prepend to file name if same name already exists in the current directory

	while(1) {	
		// Get packet from datalink layer
		Packet * pktRecvd = NULL;
		pktRecvd = datalinkRecv(pktRecvd);
		
		if ( pktRecvd == NULL ) { continue; }

		// Connection to remote host is lost
		if ( (*pktRecvd).msgType == CONN_LOST_M ) {
			fprintf( stderr, "Remote host: Connection lost.\n" );
			printf( "You can try connecting again with %s or exit with %s.\n", CONNECT, EXIT );
			isconnected = false;
			close(client_sockfd);
			break;
		}
		else if ( (*pktRecvd).msgType == CONNECT_M ) {
			printf( "Connected: socket %i\n", (*pktRecvd).sockfd );
			client_sockfd = (*pktRecvd).sockfd;
			isconnected = true;
		}
		else if ( (*pktRecvd).msgType == CHAT_M ) {
			printf( "Now chatting with: %s\n", (*pktRecvd).alias );
		}
		/*else if ( (*pktRecvd).msgType == QUIT_M ) {
			printf( "%s\n", (*pktRecvd).data );
			printf( "You can try connecting again with %s or exit with %s.\n", CONNECT, EXIT );
			isconnected = false;
			close(client_sockfd);
		}*/
		else if ( (*pktRecvd).msgType == TRANSFER_M ) {

			// First file packet received
			if ( isReceiving == false ) {

				// If file already exists in current directory, prepend a number to filename
				if ( access((*pktRecvd).filename, F_OK) != -1 ) {
					char *prepend;
					sprintf( prepend, "%i", counter );
					strncpy( fileRecvName, prepend, FILENAMESIZE );
					strcat( fileRecvName, (*pktRecvd).filename );
					counter++;
					memset( &prepend, 0, sizeof(prepend) );		// Empty the string
				}
				else {
					strncpy( fileRecvName, (*pktRecvd).filename, FILENAMESIZE );
				}

				// Notify user that file is being downloaded
				printf( "Downloading %s...\n", fileRecvName );

				// Open file for writing
				fp = fopen( fileRecvName, "ab" );
				if ( fp == NULL ) { fprintf(stderr, "File write error. Check your file name.\n"); }

				fwrite( (*pktRecvd).data, 1, sizeof((*pktRecvd).data), fp );

				// Change status to currently receiving file data
				isReceiving = true;
			}
			// Rest of the file packets
			else {
				fwrite( (*pktRecvd).data, 1, sizeof((*pktRecvd).data), fp);
			}			
		}
		else if ( (*pktRecvd).msgType == TRANSFER_END_M ) {
			fclose(fp);
			isReceiving = false;
			printf( "%s saved.\n", fileRecvName );
			memset( &fileRecvName, 0, sizeof(fileRecvName) );	// Empty the string
		}
		else if ( (*pktRecvd).msgType == MESSAGE_M ) {
			printf( "[ %s ]: %s\n", (*pktRecvd).alias, (*pktRecvd).data );
		}
		else if ( (*pktRecvd).msgType == CONFIRM_M ) {
			printf( "Now chatting with: %s\n", (*pktRecvd).alias );
		}

		memset( &pktRecvd, 0, sizeof(Packet) );		// Empty the struct
	}

	printf( "Client: receiver() thread terminated.\n" );
}

// gbn thread functions
void gbn_thread(void *param) {
	while (1) {
		if (isconnected) {
			update_gbn(&client_rcvr_gbn);
			update_gbn(&client_sndr_gbn);
		}
	}
}

void start_gbn_thread(int socketfd) {
	printf("Started client-side gbn thread\n");
    if( pthread_create(&GBNThreadData_c.thread_ID, NULL, gbn_thread, (void *)&GBNThreadData_c) ) {
		fprintf( stderr, "Error creating gbn_thread() thread. Try again.\n" );
		close( client1_sockfd );
		exit(1);
	}
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

	// Prompt client that it is now waiting to recv() on pthread
	fprintf(stdout, "Now waiting for data on [%i]...\n", client_sockfd);

	// Set isconnected flag since client successfully connected to server
	isconnected = 1;

	if (is_gbn == 1) {
		gbn_init(client_sockfd, 1, 0, WINDOWSIZE, TIMEOUT, &client_sndr_gbn);
		gbn_init(client_sockfd, 0, 0, WINDOWSIZE, TIMEOUT, &client_rcvr_gbn);

		start_gbn_thread(client_sockfd);
	    if( pthread_create(&recvthread.thread_ID, NULL, physical_receiver, (void *)&recvthread) ) {
			fprintf( stderr, "Error creating physical_receiver() thread. Try again.\n" );
			close( client_sockfd );
			exit(1);
		}
	    if( pthread_create(&newthread.thread_ID, NULL, receiver, (void *)&newthread) ) {
			fprintf( stderr, "Error creating receiver() thread. Try again.\n" );
			close( client_sockfd );
			exit(1);
		}
		// Notify user of thread start
		printf( "Client: receiver() thread started...\n" );

	}

	printf("Is connected? %d\n", isconnected);

	return 1;

	//=================================================================================

}
//=================================================================================


#endif