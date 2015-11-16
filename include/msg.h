/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Header file for supported commands
*/

#ifndef MSG_H
#define MSG_H

// Client commands
#define CONNECT		"-connect"		// Connect to server
#define CHAT		"-chat"			// Request for chat partner
#define QUIT		"-quit"			// Quit the chat channel
#define TRANSFER	"-transfer"		// Send a file to chat partner
#define HELP		"-help"			// Show available commands
#define MESSAGE		"-message"		// Send a message to chat partner
#define EXIT		"-exit"			// Terminate and exit the program
#define CONFIRM		"-confirm"		// Check with server if you are in chat queue

// Client command translations
#define CONNECT_M 	1	// Connect to server
#define CHAT_M 		2	// Request for chat partner
#define QUIT_M 		3	// Quit the chat channel
#define TRANSFER_M 	4	// Send a file to chat partner
#define HELP_M 		5	// Show available commands
#define MESSAGE_M 	6	// Send a message to chat partner
#define EXIT_M 		7	// Terminate and exit the program
#define CONFIRM_M 	8	// Check with server if you are in chat queue

#endif