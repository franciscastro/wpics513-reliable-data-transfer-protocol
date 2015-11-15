/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Client definition file
*/

#include "config.h"
#include "msg.h"
#include "client_lib.h"

// Socket file descriptor for communicating to remote host
int client_sockfd;
int clientRun = 1;

// Parse client commands
void parseCommand(char * command) {

	const char *delimiter = " ";

	// Split the command and entry
	char *token = strsep(&command, delimiter);
	char *params[2] = {0};
	params[0] = token;
	params[1] = command;

	if (strcmp(params[0], CONNECT) == 0) {
		if (params[1] == NULL) {
			printf("Usage: %s [hostname]\n", CONNECT);
			return;
		}
		client_sockfd = connectToServer();
		if (client_sockfd == -1) {
			return;
		}
	} 
	else if (strcmp(params[0], CHAT) == 0) {
		//
	} 
	else if (strcmp(params[0], QUIT) == 0) {
		//
	}
	else if (strcmp(params[0], TRANSFER) == 0) {
		//if (params[1] == NULL) {
		//	printf("Usage: %s [filename]\n", TRANSFER);
		//	return;
		//}
		//transfer(g_sockfd, params[1]);
	} 
	else if (strcmp(params[0], HELP) == 0) {
		help();
	}
	else if (strcmp(params[0], MESSAGE) == 0) {
		//if (params[1] == NULL) {
		//	printf("Usage: %s [message]\n", MESSAGE);
		//	return;
		//}
		//chat(g_sockfd, params[1]);
	}
	else if (strcmp(params[0], EXIT) == 0) {
		//grace_exit(g_sockfd);
		clientRun = 0;
	}
	else {
		printf("Invalid command: %s. \nType '%s' for command list.\n", params[0], HELP);
	}
}

void *ioHandler(void *param) {

	// For user command entries
    char user_command[USERCOMMAND];

    // Client command loop
    while (clientRun) {

    	memset(user_command, 0, USERCOMMAND);	// always clear out user_command
		printf("CLIENT> "); 					// prompt user for command
		
		if (fgets(user_command, USERCOMMAND, stdin) == NULL) {
			continue;
		}

		// Terminate string command
		user_command[strlen(user_command) - 1] = '\0';
		
		// Valid command format: -[command]
		if (user_command[0] == '-') {
			parseCommand(user_command);
			continue;
		}
		else {
			if (strcmp(removeSpace(user_command), "") == 0) {
				continue;
			}
            printf("Invalid command: %s. \nType '%s' for command list.\n", user_command, HELP);
			continue;
		}
	}

}

int main(int argc, char *argv[]) {

	// Check user arguments supplied
	if (argc != 2) {
		fprintf(stderr, "Usage: %s [gbn|sr] \n-- Where: gbn = Go-back-N, sr = Selective-repeat\n", argv[0]);
		exit(1);
	}
	// Check selected protocol and initialize datalink layer
	else if (strcmp(argv[1], "gbn") == 0 || strcmp(argv[1], "sr") == 0) {

    	printf("Protocol: %s\n", argv[1] );
    	
    	// Initialize datalink layer
    	//datalink_init(argv[1]);
    }
    // Unrecognized transfer protocol
    else {
    	fprintf(stderr, "Error: Unrecognized transfer protocol. \n");
    	exit(1);
    }

    pthread_t ioThread;
    printf("Starting client interface...\n");
	if(pthread_create(&ioThread, NULL, ioHandler, NULL) != 0) {
		fprintf(stderr, "Error: pthread_create() failed... restart client.\n");
	}

	return 0;
}
