/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Header file for shared definitions
*/

#ifndef CONFIG_H
#define CONFIG_H

#define USERCOMMAND 100

#define HOSTNAME "wind"
#define PORT "3490" 		// The port client will be connecting to
#define WINDOWSIZE 5		// Window size for transfer protocol
#define MAXDATASIZE 1000 	// max number of bytes we can get at once
#define DROPRATE 50			// % of packets to be dropped
#define CORRUPTRATE 50		// % of packets corrupted
#define FILENAMESIZE 30		// Max size of filenames
#define MAXCOMMANDSIZE 30	// Max size of commands
#define MAXDATA 100			// Max size of data

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

struct packet {
	char command[MAXCOMMANDSIZE];	// Command triggered
	char message[MAXDATA];			// Data
	char alias[MAXCOMMANDSIZE];		// Client alias
	char filename[MAXCOMMANDSIZE];	// File name if needed
	int filebytesize;				// Size in bytes of message
};

#endif