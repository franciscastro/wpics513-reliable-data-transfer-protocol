/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Header file for supported commands
*/

#ifndef MSG_H
#define MSG_H

// Client commands
#define CONNECT "-connect"		// Connect to server
#define CHAT "-chat"			// Request for chat partner
#define QUIT "-quit"			// Quit the chat channel
#define TRANSFER "-transfer"	// Send a file to chat partner
#define HELP "-help"			// Show available commands
#define MESSAGE "-message"		// Send a message to chat partner
#define EXIT "-exit"			// Terminate and exit the program
#define CONFIRM "-confirm"		// Check with server if you are in chat queue

#endif