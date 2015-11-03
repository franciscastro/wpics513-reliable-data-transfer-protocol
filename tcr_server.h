/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 1 - Chat Roulette
Last modified: 12 Oct 2015

This is the TCR server header file.
*/

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
/*
	file sending commands
	---------------------

	FILE_ACCEPT
	FILE_RESPONSE
	FILE_SEND
*/

/*
	CHAT_PROMPT
	CHAT_RESPONSE
*/

/*
TO DO:
- ACKN
- IN_SESSION
*/


//struct containing data
// struct packet
// {
// 	char client_alias[ALIAS_LEN];
// 	char command[COMMAND_LEN];
// 	char message[MSG_LEN];
// };

struct packet {
	char command[MAXCOMMANDSIZE];	// Command triggered
	char message[MAXMESSAGESIZE];	// Data
	char alias[MAXCOMMANDSIZE];		// Client alias
	char filename[MAXCOMMANDSIZE];	// File name if needed
	int filebytesize;	
};

struct process_info
{
	int pid;
	int socketfd;
	char client_alias[ALIAS_LEN];
};

struct client_data
{
	char client_alias[ALIAS_LEN];
	int socketfd;
	int connected_socket;
};


/*
	Global vars
	-----------
*/

int ready;
int client_map[MAX_CLIENTS];
int client_flags[MAX_CLIENTS];
int client_blocks[MAX_CLIENTS];
int client_bytes[MAX_CLIENTS];
char client_alias[MAX_CLIENTS][MAXCOMMANDSIZE];
char client_file_addresses[MAX_CLIENTS][MAXCOMMANDSIZE];

void disconnect_chat(int socketfd, int reset_a);
void disconnect_chat_both(int socket_from, int socket_to);

/*
	General helper functions
*/
// ====================================================================

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
		client_map[socketfd] = -2;
		client_flags[socketfd] = 0;
		client_blocks[socketfd] = 0;
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

int compare_command(struct packet *msgrecvd, char* cmd) {
	return (strcmp(msgrecvd->command, cmd) == 0); 
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
	}
}

void connect_confirm(int socketfd) {
	struct packet response;
	strcpy(response.command, "ACKN");

	if (socketfd >= MAX_CLIENTS) {
		perror("Socket from number exceeds maximum socket amount");
		strcpy(response.message, "Chat queue full, unable to add user");
	}
	else {
		if (client_map[socketfd] == socketfd || client_map[socketfd] == -1) {
			printf("User on socket %d is on the chat queue\n", socketfd);
			strcpy(response.message, "You are on the chat queue");
		}
		else if(client_map[socketfd] != -2) {
			printf("User on socket %d is on a chat channel\n", socketfd);
			strcpy(response.message, "You are on a chat channel");
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

int send_data(int socket_to, struct packet *packet) {
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

	memset(packet, 0, sizeof(struct packet));
	return n == -1 ? -1 : 0;	// Return 0 on success, -1 on failure
}

void random_chat_connect(int socket_from, int socket_to, char* from_alias) {
	if (socket_from >= MAX_CLIENTS || socket_to >= MAX_CLIENTS) {
		perror("Invalid socket number/s for request");
		return;
	}

	assign_client_alias_local(socket_from, socket_to, from_alias);
	strcpy(client_alias[socket_from], from_alias);
	client_map[socket_to] = socket_from;
	client_map[socket_from] = socket_to;

	struct packet to_packet, from_packet;
	strcpy(to_packet.command, "IN SESSION");
	strcpy(from_packet.command, "IN SESSION");

	strcpy(to_packet.message, "You, ");
	strcat(to_packet.message, client_alias[socket_to]);
	strcat(to_packet.message, ", are chatting with ");
	strcat(to_packet.message, client_alias[socket_from]);

	strcpy(from_packet.message, "You, ");
	strcat(from_packet.message, client_alias[socket_from]);
	strcat(from_packet.message, ", are chatting with ");
	strcat(from_packet.message, client_alias[socket_to]);

	if (send_data(socket_to, &to_packet) == -1)
		perror("send");
	if (send_data(socket_from, &from_packet) == -1)
		perror("send");
}

int random_chat_init(int socket_from, char* alias) {
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

		random_chat_connect(socket_from, prospect_socket, alias);
	}
	else {
		client_map[socket_from] = socket_from;
		strcpy(client_alias[socket_from], alias);
		struct packet to_packet, from_packet;
		strcpy(from_packet.command, "NOTIF");
		strcpy(from_packet.message, "Ready to chat. Waiting for other users...");

		if (send_data(socket_from, &from_packet) == -1)
			perror("send");
	}
	return 1;
}

void disconnect_chat(int socketfd, int reset_a) {
	if (socketfd > 0 && client_map[socketfd] > -1) {
		printf ("Disconnecting chat channel of socket %d\n", socketfd);
		struct packet response;
		strcpy(response.command, "QUIT");
		if (client_map[socketfd] != socketfd){
			strcpy(response.message, "Disconnected from chat with ");
			strcat(response.message, client_alias[client_map[socketfd]]);
		}
		else{
			printf ("User %d ready-to-chat status has been garbaged\n", socketfd);
			strcpy(response.message, "User ready-to-chat status has been garbaged");
		}
		strcat(response.message, "\nUser put back into chat queue");
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

void flag(int socketfd) {
	client_flags[socketfd]++;
}

void handle_chat_message(int socket_from, struct packet *msgrecvd) {
	struct packet to_send;

	printf ("%d\n", client_map[socket_from]);
	if (client_map[socket_from] < 0 || client_map[socket_from] == socket_from) {
		strcpy(to_send.command, "NOTIF");
		strcpy(to_send.alias, "Server");
		strcpy(to_send.message, "You have not been connected to chat");

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

void parse_recvd_data(int socketfd, struct packet *msgrecvd) {
	if (compare_command(msgrecvd, "CHAT")) {
		if (client_blocks[socketfd] > 0){
			struct packet to_send;

			strcpy(to_send.command, "NOTIF");
			strcpy(to_send.alias, "Server");
			strcpy(to_send.message, "You have been blocked; your chat privileges have been suspended");
			if (send_data(socketfd, &to_send) == -1)
				perror("send");
		}
		else{
			int prospect_socket = random_chat_init(socketfd, msgrecvd->alias);
			if (prospect_socket < 1) {
				struct packet to_send;

				strcpy(to_send.command, "NOTIF");
				strcpy(to_send.alias, "Server");
				strcpy(to_send.message, "Chatroulette request failed");
				if (send_data(socketfd, &to_send) == -1)
					perror("send");
			}
		}
	}
	else if(compare_command(msgrecvd, "CONFIRM")){
		connect_confirm(socketfd);
	}
	else if (compare_command(msgrecvd, "QUIT")) {
		disconnect_chat_both(socketfd, client_map[socketfd]);
	}
	else if (compare_command(msgrecvd, "MESSAGE")) {
		handle_chat_message(socketfd, msgrecvd);
	}
	else if (compare_command(msgrecvd, "TRANSFER")) {
		if (client_map[socketfd] != -1 && client_map[socketfd] != socketfd) {
			strcpy(msgrecvd->command, "FILE_SEND");
			if (send_data(client_map[socketfd], msgrecvd) == -1)
				perror("send");
			else {
				client_bytes[socketfd] += sizeof(*msgrecvd);
				client_bytes[client_map[socketfd]] += sizeof(*msgrecvd);
			}
		}
		else {
			struct packet to_send;
			strcpy(to_send.command, "NOTIF");
			strcpy(to_send.alias, "Server");
			strcpy(to_send.message, "Unauthorized file send");
			if (send_data(client_map[socketfd], &to_send) == -1)
				perror("send");
		}
	}
	else if (compare_command(msgrecvd, "TRANSFER_END")) {
		if (client_map[socketfd] != -1 && client_map[socketfd] != socketfd){
			strcpy(msgrecvd->command, "FILE_END");
			if (send_data(client_map[socketfd], msgrecvd) == -1)
				perror("send");
			else {
				client_bytes[socketfd] += sizeof(*msgrecvd);
				client_bytes[client_map[socketfd]] += sizeof(*msgrecvd);
			}
		}
		else {
			struct packet to_send;
			strcpy(to_send.command, "NOTIF");
			strcpy(to_send.alias, "Server");
			strcpy(to_send.message, "Unauthorized file send");
			if (send_data(client_map[socketfd], &to_send) == -1)
				perror("send");
		}
	}
	else if (compare_command(msgrecvd, "FLAG")) {
		struct packet response;
		strcpy(response.command, "NOTIF");
		strcpy(response.alias, "Server");
		if (client_map[socketfd] == socketfd || client_map[socketfd] < 0){
			strcpy(response.message, "You cannot flag outside a chat channel");
		}
		else{
			strcpy(response.message, "Flagged your partner ");
			strcat(response.message, client_alias[socketfd]);
			flag(client_map[socketfd]);
		}
		if (send_data(socketfd, &response) == -1)
			perror("send");
	}
	else if (compare_command(msgrecvd, "HELP")){
		struct packet help_packet;
		strcpy(help_packet.command, "HELP");
		strcpy(help_packet.alias, "Server");
		strcpy(help_packet.message,
				"CONNECT - Connect to the Text ChatRoulette server\nCHAT - Request the Text ChatRoulette server to connect with another client to chat\nQUIT - Quit the current chat channel\nTRANSFER - Send a file to the other client on the current chat channel\nFLAG - Report the current chatting partner to the Text ChatRoulette server\nHELP - Display this command list\nMESSAGE - Send a message to the current chatting partner\nEXIT - Disconnect from the Text ChatRoulette server and quit the client program\nCONFIRM - Check with the Text ChatRoulette server if you are in the chat queue\n");
		if (send_data(socketfd, &help_packet) == -1)
			perror("send");
	}
}

void handle_client_adv(int socketfd, fd_set *master) {
	// printf("got master %d\n", master);
	int nbytes;
	char buf[BUF_LEN];

	struct packet msgrecvd;
	if ((nbytes = recv(socketfd, (void *)&msgrecvd, sizeof(struct packet), 0)) <= 0) {
		if (nbytes == 0)
            printf("selectserver: socket %d hung up\n", socketfd);
        else 
            perror("recv");
        // simple socket close
        shutdown_socket(socketfd, SO_KEEPALIVE, 1, master);
	}
    else {
		printf("Received command: %s from %d\n", msgrecvd.command, socketfd);
		parse_recvd_data(socketfd, &msgrecvd);
    }
}

/*  
	for fork()-ed implementation
	not to be used with listen()
*/
void handle_client(int socketfd, int pid, char* buf, int nbytes)
{
	struct packet data;
	int bytes, sent;
	printf("handling client %d using pid %d\n",socketfd, pid);
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{	
		if (client_map[i] == i && i != socketfd) {
			if (send(i, buf, nbytes, 0) == -1)
				perror("send");
			printf("socket %d is connected!\n", i);
		}
	}
}

void handle_client_fork(int socketfd, fd_set *master, int pid) {
	// printf("got master %d\n", master);
	int nbytes;
	char buf[BUF_LEN];
	if ((nbytes = recv(socketfd, buf, sizeof buf, 0)) <= 0) {
		if (nbytes == 0)
            printf("selectserver: socket %d hung up\n", socketfd);
        else 
            perror("recv");
        close(socketfd); // close() this descriptor
        FD_CLR(socketfd, master); // Remove from master set
        kill(pid, SIGKILL);
	}
    else
		handle_client(socketfd, pid, buf, nbytes);
}
