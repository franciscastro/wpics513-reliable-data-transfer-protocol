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

// [USER INPUT]
#define USERCOMMAND 70

// [CONNECTION]
#define HOSTNAME	"CS"	// The target hostname
#define PORT 		"3490" 	// The port client will be connecting to

// [TRANSFER PROTOCOL]
#define WINDOWSIZE 	5		// Window size for transfer protocol
#define DROPRATE 	50		// % of packets to be dropped
#define CORRUPTRATE 50		// % of packets corrupted

// [APPLICATION DATA]
#define MAXDATASIZE 	1000 	// Max number of bytes we can get at once
#define FILENAMESIZE	20		// Max size of filenames
#define MAXCOMMANDSIZE	30		// Max size of commands
#define MAXDATA			70		// Max size of data

// [DATALINK]
#define GOBACKN		1	// Go-back-N transfer protocol
#define SELREPEAT	2	// Selective repeat transfer protocol

// [CLIENT COMMANDS]
#define CONNECT		"-connect"		// Connect to server
#define CHAT		"-chat"			// Request for chat partner
#define QUIT		"-quit"			// Quit the chat channel
#define TRANSFER	"-transfer"		// Send a file to chat partner
#define HELP		"-help"			// Show available commands
#define MESSAGE		"-message"		// Send a message to chat partner
#define EXIT		"-exit"			// Terminate and exit the program
#define CONFIRM		"-confirm"		// Check with server if you are in chat queue

// [CLIENT COMMAND TRANSLATIONS]
#define CONNECT_M 	1	// Connect to server
#define CHAT_M 		2	// Request for chat partner
#define QUIT_M 		3	// Quit the chat channel
#define TRANSFER_M 	4	// Send a file to chat partner
#define HELP_M 		5	// Show available commands
#define MESSAGE_M 	6	// Send a message to chat partner
#define EXIT_M 		7	// Terminate and exit the program
#define CONFIRM_M 	8	// Check with server if you are in chat queue

// [PACKET TYPES]
#define MSG_PKT		1	// User data packet
#define ACK_PKT		2	// ACK data packet

/*struct packet {
	char command[MAXCOMMANDSIZE];	// Command triggered
	char message[MAXDATA];			// Data
	char alias[MAXCOMMANDSIZE];		// Client alias
	char filename[MAXCOMMANDSIZE];	// File name if needed
	int filebytesize;				// Size in bytes of message
};*/

struct AppMessage {
	int messageType;
	char filename[FILENAMESIZE];
	char data[];
};

struct Packet {
	int packetType;
	int seqNumber;
	AppMessage appMsg;
};

struct Frame {
	int checksum;
	Packet pkt;
};

#endif