/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Client file.
*/

#include "rdt_client.h"

// Main loop for the client
int main(int argc, char *argv[]) {

	// Convert corrupt rate and drop rate parameters to int
	int corruptRate = atoi(argv[2]);
	int dropRate = atoi(argv[3]);

	// Check user arguments supplied
	if ( argc != 4 ) {
		fprintf(stderr, "Usage: %s [gbn|sr] [corrupt rate] [drop rate]\n-- Where: gbn = Go-back-N, sr = Selective-repeat\n", argv[0]);
		exit(1);
	}
	// Check selected protocol and initialize datalink layer
	else if ( strcmp(argv[1], "gbn") == 0 || strcmp(argv[1], "sr" ) == 0) {
    	printf( "Protocol: %s\n", argv[1] );
    }
    // Unrecognized transfer protocol
    else {
    	fprintf( stderr, "Error: Unrecognized transfer protocol. \n" );
    	exit(1);
    }
    
    // Check if corrupt rate and drop rate are within bounds
    if ( ( corruptRate < 0 || corruptRate > 100 ) || ( dropRate < 0 || dropRate > 100 ) ) {
    	printf( "Corrupt and drop rates must be >= 0 and <= 100.\n" );
    	exit(1);
    }
    

    char user_command[USERCOMMAND];		// For user command entries

    // Client main loop waits for input from user
    while (1) {

    	memset(user_command, 0, USERCOMMAND);	// always clear out user_command
		printf("CLIENT> "); 					// prompt user for command
		
		if (fgets(user_command, USERCOMMAND, stdin) == NULL) {
			continue;
		}

		// Terminate string command
		user_command[strlen( user_command ) - 1] = '\0';
		
		// Valid command format: -[command]
		if ( user_command[0] == '-' ) {
			parseCommand( user_command );
			continue;
		}
		else {
			if ( strcmp( removeSpace( user_command ), "" ) == 0 ) {
				continue;
			}
            printf( "Invalid command: %s. \nType '%s' for command list.\n", user_command, HELP );
			continue;
		}
	}

	return 0;
}
