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
#define MAXCOMMANDSIZE	30		// Max size of commands
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
	CONN_LOST_M		// Connection to remote host is lost
} MessageType;

// Event type
typedef enum {
	FRAME_ARRIVAL, 
	CKSUM_ERR, 
	TIMEOUT, 
	UPPER_LAYER_READY,
	ACK_TIMEOUT
} EventType;

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
	char data[MAXDATA];
} Packet;

// Datalink frame
typedef struct Frame {
	int checksum;
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

BufferEntry * fromClient;   // Pointer to outgoing buffer for storing packets from client
BufferEntry * forClient;    // Pointer to incoming buffer for storing packets received from physical layer

#endif