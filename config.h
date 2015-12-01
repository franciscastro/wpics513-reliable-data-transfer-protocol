/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Header file for shared definitions
*/

#ifndef CONFIG_H_
#define CONFIG_H_

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
#include <sys/stat.h>

// [ CONNECTION ]
#define HOSTNAME	"CS"	// The target hostname
#define PORT 		"3490" 	// The port client will be connecting to

// [ TRANSFER PROTOCOL ]
#define WINDOWSIZE 	5		// Window size for transfer protocol
#define TIMEOUT 1.0

// [ DATALINK LAYER ]
#define GOBACKN		1	// Go-back-N transfer protocol
#define SELREPEAT	2	// Selective repeat transfer protocol

// [ CLIENT / APPLICATION LAYER ]
#define CONNECT		"-connect"		// Connect to server
#define CHAT		"-chat"			// Request for chat partner
#define QUIT		"-quit"			// Quit the chat channel
#define TRANSFER	"-transfer"		// Send a file to chat partner
#define HELP		"-help"			// Show available commands
#define MESSAGE		"-message"		// Send a message to chat partner
#define EXIT		"-exit"			// Terminate and exit the program
#define CONFIRM		"-confirm"		// Check with server if you are in chat queue
#define MAXDATASIZE 	1000 	// Max number of bytes we can get at once
#define FILENAMESIZE	30		// Max size of filenames
#define MAXCOMMANDSIZE	100		// Max size of commands
#define ALIASSIZE		30		// Max size of client alias
#define MAXDATA			100		// Max size of data
#define USERINPUT 		100		// Max size of user input

// Boolean type
typedef enum {
	false, 
	true
} boolean;

// Message type
typedef enum {
	CONNECT_M,		// Connect to server
	CHAT_M,			// Request for chat partner
	QUIT_M,			// Quit the chat channel
	TRANSFER_M,		// Send a file to chat partner
	HELP_M,			// Show available commands
	MESSAGE_M,		// Send a message to chat partner
	EXIT_M,			// Terminate and exit the program
	CONFIRM_M,		// Check with server if you are in chat queue
	TRANSFER_END_M,	// Flag end of file transfer
	CONN_LOST_M	,	// Connection to remote host is lost
	IN_SESSION_M,	// Currently chatting
	ACK_M,			// ACK for datalink layer to process as ack
	SPAM_M
} MessageType;

// IGNORE: USED IN FRANCIS' TRANSFER PROTOCOL IMPLEMENTATION
// Event type
/*typedef enum {
	FRAME_ARRIVAL, 
	CKSUM_ERR, 
	TIMEOUT, 
	UPPER_LAYER_READY,
	ACK_TIMEOUT
} EventType;*/
	
// Frame type
typedef enum {
	DATA_F, 
	ACK_F
} FrameType;

// Application packet
typedef struct Packet {
	MessageType msgType;
	int sockfd;
	char alias[ALIASSIZE];
	char filename[FILENAMESIZE];
	int filebytesize;
	char data[MAXDATA];
} Packet;

// Datalink frame
typedef struct Frame {
	unsigned char checksum[8];
	int seqNumber;
	int nextAckExpected;
	FrameType type;
	Packet pkt;
} Frame;

// Structure definition for a temporary buffer space to store Packets received
typedef struct BufferEntry {
    struct BufferEntry * next;
    Packet pkt;
} BufferEntry;

void destroy_pbe(BufferEntry * pbe) {
	if (pbe == NULL)
		return;

	if (pbe->next != NULL)
		free(pbe->next);
	pbe->next = NULL;
	free(pbe);
}

BufferEntry * fromClient;   // Pointer to outgoing buffer for storing packets from client
BufferEntry * forClient;    // Pointer to incoming buffer for storing packets received from physical layer

BufferEntry * fromServer1;   // Pointer to outgoing buffer for storing packets from client
BufferEntry * forServer1;    // Pointer to incoming buffer for storing packets received from physical layer

BufferEntry * fromServer2;   // Pointer to outgoing buffer for storing packets from client
BufferEntry * forServer2;    // Pointer to incoming buffer for storing packets received from physical layer

typedef struct FrameBufferEntry {
	struct FrameBufferEntry * next;
	struct FrameBufferEntry * prev;
	Frame frame;
	int count;
} FrameBufferEntry;

void destroy_fbe(FrameBufferEntry * fbe) {
	if (fbe == NULL)
		return;

	if (fbe->next != NULL)
		free(fbe->next);
	if (fbe->prev != NULL)
		free(fbe->prev);
	fbe->next = NULL;
	fbe->prev = NULL;
	free(fbe);
}

#endif