#ifndef RDT_SERVER_H_
#define RDT_SERVER_H_

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>

#include "gbn.h"

#define BACKLOG 20	// how many pending connections queue will hold
#define PORT "3490" // the port users will be connecting to

#define BUF_LEN 2048
#define MSG_LEN 1024
#define COMMAND_LEN 20
#define ALIAS_LEN 64

#define MAX_CLIENTS 256
#define MAXMESSAGESIZE 1024
#define MAXCOMMANDSIZE 20

#define OPTLEN 16

int ready;
int client_map[MAX_CLIENTS];
int client_bytes[MAX_CLIENTS];
char client_alias[MAX_CLIENTS][MAXCOMMANDSIZE];
char client_file_addresses[MAX_CLIENTS][MAXCOMMANDSIZE];

int is_gbn, client1_connected, client2_connected, client1_chatting, client2_chatting;

GBN_instance client1_sndr_gbn;
GBN_instance client1_rcvr_gbn;
GBN_instance client2_sndr_gbn;
GBN_instance client2_rcvr_gbn;

/*
	Faux-client data (assume max: 2 clients)
*/
typedef struct ThreadData {
	pthread_t thread_ID;		// This thread's pointer
	int socketfd;
	int socket_id;
} ThreadData;

ThreadData newthread1;			// client1 receiver() thread
ThreadData newthread2;			// client2 receiver() thread

/*
	Faux-client data (assume max: 2 clients)
*/
GBNThreadData GBNThreadData1;			// client1 gbnthread1 thread
GBNThreadData GBNThreadData2;			// client2 gbn thread

struct addrinfo hints1, hints2, *servinfo1, *servinfo2, *p1, *p2;
int * error_status1, error_status2;

void disconnect_chat(int socketfd, int reset_a);
void disconnect_chat_both(int socket_from, int socket_to);

/*
	Utility functions
*/
char* removeSpace(char *s);
void allCaps(char *command);
off_t filesize(const char *filename);
void *get_in_addr(struct sockaddr *sa);
void parseCommand(char * command);
void createMessage(const char* command, const char* message);
void sendFilePackets(const char* filename);
void help();
void *receiver(void *param);

/*
	Helper functions
*/
int reset_alias(int socketfd) {
	if (socketfd >= MAX_CLIENTS) {
		perror("Invalid socket");
		return -1;
	}
	memset(&client_alias[socketfd], 0, MAXCOMMANDSIZE*sizeof(client_alias[0]));
	return 1;
}

void reset_aliases() {
	int c;
	for (c = 0; c < MAX_CLIENTS; c++)
		reset_alias(c);
}

void reset_client_maps(){
	int c;
	for (c = 0; c < MAX_CLIENTS; c++)
		client_map[c] = -2;	
}

void reset_socket(int socketfd) {
	printf("reseting socket %d\n", socketfd);
	if (reset_alias(socketfd) == 1) {
		disconnect_chat(client_map[socketfd], 1);
	}
}

int shutdown_socket(int socketfd, int flag, int free_socket, fd_set *master) {
	// for freeing socket descriptor	
	reset_socket(socketfd);
	if (free_socket) {
        close(socketfd); // close() this descriptor
        FD_CLR(socketfd, master); // Remove from master set
		return 1;
	}

	// change socket usability with shutdown
	int success = shutdown(socketfd, flag);
	if (success == 0) {
        FD_CLR(socketfd, master); // Remove from master set
		return 1;
	}

	return 0;
}

void sigchld_handler(int s) {
	
	// waitpid() might overwrite errno, save and restore it
	int saved_errno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

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

/*
	Server functions
*/
// ====================================================================
void connect_client(int socketfd) {
	if (socketfd >= MAX_CLIENTS) {
		perror("Socket from number exceeds maximum socket amount");
	}
	else {
		printf("Added user on socket %d to chat queue\n", socketfd);
		client_map[socketfd] = -1;

		if (client1_connected == 0) {
			client1_connected = 1;
			client1_sockfd = socketfd;

			gbn_init(socketfd, 1, 1, WINDOWSIZE, TIMEOUT, &client1_sndr_gbn);
			gbn_init(socketfd, 0, 1, WINDOWSIZE, TIMEOUT, &client1_rcvr_gbn);
		}
		else {
			client2_connected = 1;
			client2_sockfd = socketfd;

			gbn_init(socketfd, 1, 1, WINDOWSIZE, TIMEOUT, &client2_sndr_gbn);
			gbn_init(socketfd, 0, 1, WINDOWSIZE, TIMEOUT, &client2_rcvr_gbn);

		}
		start_thread(socketfd);
		start_gbn_thread(socketfd);	
	}
}

void connect_confirm(int socketfd) {
	Packet response;
	response.msgType = ACK_M;

	if (socketfd >= MAX_CLIENTS) {
		perror("Socket from number exceeds maximum socket amount");
		strcpy(response.data, "Chat queue full, unable to add user");
	}
	else {
		if (client_map[socketfd] == socketfd || client_map[socketfd] == -1) {
			printf("User on socket %d is on the chat queue\n", socketfd);
			strcpy(response.data, "You are on the chat queue");
		}
		else if(client_map[socketfd] != -2) {
			printf("User on socket %d is on a chat channel\n", socketfd);
			strcpy(response.data, "You are on a chat channel");
		}
	}
	if (send_data(socketfd, &response) == -1)
		perror("send");
}

// assume socket fd2 already has a connection
void assign_client_alias_local(int socketfd1, int socketfd2, char* alias) {
	if (socketfd1 >= MAX_CLIENTS || socketfd2 >= MAX_CLIENTS) {
		perror("Invalid socket ids");
		return;
	}

	char* fd2_alias = &client_alias[socketfd2][0];

	int cmp = strcmp(alias, fd2_alias);
	if (cmp == 0) {
		alias = strcat(alias, "(1)");
		printf("Duplicate alias. Assigning new alias %s\n", alias);
	}
}

int send_data(int socket_to, Packet *packet) {
	if (socket_to < 0)
		return 1;
	
	int packetlen = sizeof *packet;
	int total = 0;				// How many bytes we've sent
    int bytesleft = packetlen;	// How many we have left to send
    int n;

    // To make sure all data is sent
    while(total < packetlen) {

        n = send(socket_to, (packet + total), bytesleft, 0);
        
        if (n == -1) { break; }
        
        total += n;
        bytesleft -= n;

    }

    packetlen = total;	// Return number actually sent here

	memset(packet, 0, sizeof(Packet));
	return n == -1 ? -1 : 0;	// Return 0 on success, -1 on failure
}

void chat_connect(int socket_from, int socket_to, char* from_alias) {
	if (socket_from >= MAX_CLIENTS || socket_to >= MAX_CLIENTS) {
		perror("Invalid socket number/s for request");
		return;
	}

	assign_client_alias_local(socket_from, socket_to, from_alias);
	strcpy(client_alias[socket_from], from_alias);
	client_map[socket_to] = socket_from;
	client_map[socket_from] = socket_to;

	Packet to_packet, from_packet;
	to_packet.msgType = IN_SESSION_M;
	from_packet.msgType = IN_SESSION_M;

	strcpy(to_packet.data, "You, ");
	strcat(to_packet.data, client_alias[socket_to]);
	strcat(to_packet.data, ", are chatting with ");
	strcat(to_packet.data, client_alias[socket_from]);

	strcpy(from_packet.data, "You, ");
	strcat(from_packet.data, client_alias[socket_from]);
	strcat(from_packet.data, ", are chatting with ");
	strcat(from_packet.data, client_alias[socket_to]);

	// set as gbn send
	if (send_data(socket_to, &to_packet) == -1) {
		perror("send");
	}
	if (send_data(socket_from, &from_packet) == -1) {
		perror("send");
	}

	if (socket_from == client1_sockfd) 
		client1_chatting = true;
	else if(socket_from == client2_sockfd)
		client2_chatting = true;
}

int chat_init(int socket_from, char* alias) {
	if (socket_from >= MAX_CLIENTS) {
		perror("Socket from number exceeds maximum socket amount");
		return -1;
	}

	int i;
	int connected_count = 0;
	int prospect_socket = -1;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (client_map[i] == i)
			connected_count++;
	}

	if (connected_count > 0) {
		int idx = 0;
		int prospect_socket = -1;
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (client_map[i] == i && i != socket_from) {
				prospect_socket = i;
				break;
			}
		}

		chat_connect(socket_from, prospect_socket, alias);
	}
	else {
		client_map[socket_from] = socket_from;
		strcpy(client_alias[socket_from], alias);
		Packet from_packet;
		from_packet.msgType = CONFIRM_M;
		strcpy(from_packet.data, "Ready to chat. Waiting for other users...");
		if (send_data(socket_from, &from_packet) == -1) // switch to gbn / srpt send
			perror("send");
	}
	return 1;
}

void disconnect_chat(int socketfd, int reset_a) {
	if (socketfd > 0 && client_map[socketfd] > -1) {
		printf ("Disconnecting chat channel of socket %d\n", socketfd);
		Packet response;
		response.msgType = QUIT_M;
		if (client_map[socketfd] != socketfd){
			strcpy(response.data, "Disconnected from chat with ");
			strcat(response.data, client_alias[client_map[socketfd]]);
		}
		else{
			printf ("User %d ready-to-chat status has been garbaged\n", socketfd);
			strcpy(response.data, "User ready-to-chat status has been garbaged");
		}
		strcat(response.data, "\nUser put back into chat queue");
		if (send_data(socketfd, &response) == -1)
			perror("send");

		client_map[socketfd] = -1;
		client_bytes[socketfd] = 0;

		if (reset_a == 1)
			reset_alias(socketfd);
	}
}
	
void disconnect_chat_both(int socket_from, int socket_to) {
	disconnect_chat(socket_from, 0);	
	disconnect_chat(socket_to, 0);
	reset_alias(socket_from);
	reset_alias(socket_to);
}

void handle_chat_message(int socket_from, Packet *msgrecvd) {
	Packet to_send;

	printf ("%d\n", client_map[socket_from]);
	if (client_map[socket_from] < 0 || client_map[socket_from] == socket_from) {
		to_send.msgType = CONFIRM_M;
		strcpy(to_send.alias, "Server");
		strcpy(to_send.data, "You have not been connected to chat");

		if (send_data(socket_from, &to_send) == -1)
			perror("send");
	}
	else {
		to_send = *msgrecvd;
		client_bytes[socket_from] += sizeof(*msgrecvd);
		client_bytes[client_map[socket_from]] += sizeof(*msgrecvd);

		if (send_data(client_map[socket_from], &to_send) == -1)
			perror("send");

	}
}

void handle_client_adv(int socketfd, fd_set *master) {
	int nbytes;
	char buf[BUF_LEN];

	Frame msgrecvd;
	nbytes = recv(socketfd, (void *)&msgrecvd, sizeof(Frame), 0);

	if (nbytes <= 0) {
		if (nbytes == 0)
            printf("selectserver: socket %d hung up\n", socketfd);
        else 
            perror("recv");
        // simple socket close
        shutdown_socket(socketfd, SO_KEEPALIVE, 1, master);
	}
    else {
		if (socketfd == client1_sockfd) {
			gbn_rdt_rcv(&client1_rcvr_gbn, msgrecvd, socketfd);
		}
		else if (socketfd == client2_sockfd){
			gbn_rdt_rcv(&client2_rcvr_gbn, msgrecvd, socketfd);
		}
		// parse_recvd_data(socketfd, &msgrecvd); //->replace with gbn/datalink rcv
    }
}

/*
	Application layer recvr thread:  For recv()ing messages from the socket
*/
void *receiver1(void *param) {
	printf("Starting client1 receiver thread\n");

	// File variables
	FILE *fp;						// File pointer
	boolean isReceiving = false;	// Flag if client is currently receiving a file
	char *fileRecvName;				// File name of file currently being received
	int counter = 0;				// Prepend to file name if same name already exists in the current directory

	while(1) {

		// Get packet from datalink layer
		Packet * pktRecvd = extract_data(1, client1_sockfd);

		//deliver_data(1, client_sockfd1); //change to get data from packet buffer
		
		if ( pktRecvd == NULL ) { continue; }

		if ( (*pktRecvd).msgType == CHAT_M) {
			printf("Received a chat command!\n");
		}
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
			printf( "%s\n", (*pktRecvd).data );
		}

		memset( &pktRecvd, 0, sizeof(Packet) );		// Empty the struct
	}

	printf( "Client: receiver() thread terminated.\n" );
}

void *receiver2(void *param) {

	// File variables
	FILE *fp;						// File pointer
	boolean isReceiving = false;	// Flag if client is currently receiving a file
	char *fileRecvName;				// File name of file currently being received
	int counter = 0;				// Prepend to file name if same name already exists in the current directory

	while(1) {

		// Get packet from datalink layer
		Packet * pktRecvd = extract_data(1, client2_sockfd);
		
		if ( pktRecvd == NULL ) { continue; }

		if ( (*pktRecvd).msgType == CHAT_M) {
			printf("Received a chat command!\n");
		}
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
			printf( "%s\n", (*pktRecvd).data );
		}

		memset( &pktRecvd, 0, sizeof(Packet) );		// Empty the struct
	}

	printf( "Client: receiver() thread terminated.\n" );
}

/*
	Function to start thread
*/
void start_thread(int socket_id) {
	if (socket_id == client1_sockfd) {
	    if( pthread_create(&newthread1.thread_ID, NULL, receiver1, (void *)&newthread1) ) {
			fprintf( stderr, "Error creating receiver() thread. Try again.\n" );
			close( client1_sockfd );
			exit(1);
		}
		else {
			fcntl(client1_sockfd, O_NONBLOCK);
		}
	}
	else if(socket_id == client2_sockfd){
	    if( pthread_create(&newthread2.thread_ID, NULL, receiver2, (void *)&newthread2) ) {
			fprintf( stderr, "Error creating receiver() thread. Try again.\n" );
			close( client2_sockfd );
			exit(1);
		}
		else {
			fcntl(client2_sockfd, O_NONBLOCK);
		}
	}
}

void gbn_thread_1(void *param) {
	while (1) {
		if (client1_connected) {
			update_gbn(&client1_rcvr_gbn);
			update_gbn(&client1_sndr_gbn);
		}
	}
}

void gbn_thread_2(void *param) {
	while(1) {
		if (client2_connected) {
			update_gbn(&client2_rcvr_gbn);
			update_gbn(&client2_sndr_gbn);
		}
	}
}

void start_gbn_thread(int socketfd) {
	if (socketfd == client1_sockfd) {
		printf("started client1 gbn thread\n");
	    if( pthread_create(&GBNThreadData1.thread_ID, NULL, gbn_thread_1, (void *)&GBNThreadData1) ) {
			fprintf( stderr, "Error creating gbn_thread_1() thread. Try again.\n" );
			close( client1_sockfd );
			exit(1);
		}
	}
	else if (socketfd == client2_sockfd) {
		printf("started client2 gbn thread\n");
	    if( pthread_create(&GBNThreadData2.thread_ID, NULL, gbn_thread_2, (void *)&GBNThreadData2) ) {
			fprintf( stderr, "Error creating gbn_thread_2() thread. Try again.\n" );
			close( client2_sockfd );
			exit(1);
		}
	}
}

#endif