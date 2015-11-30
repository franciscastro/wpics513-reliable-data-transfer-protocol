/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 1 - Chat Roulette
Last modified: 12 Oct 2015

This is the TCR server process file.
*/

#include "rdt_server.h"
int enable_stats;
int abs_start;

// throws a socket out of its existing connections/channels
void throwout(int socketfd) {
	if (client_map[socketfd] != socketfd && socketfd > -1) {
		// construct response packet
		Packet throw_packet, throw_notif_packet;
		throw_packet.msgType = CONFIRM_M;
		throw_notif_packet.msgType = CONFIRM_M;

		strcpy(throw_packet.data, "Server has thrown you out of your chat channel with ");
		strcat(throw_packet.data, client_alias[client_map[socketfd]]);

		strcpy(throw_notif_packet.data, "Server has thrown your partner ");
		strcat(throw_notif_packet.data, client_alias[socketfd]);
		strcat(throw_notif_packet.data, " out of your chat channel");

		// replace with datalink sending
		if (send_data(socketfd, &throw_packet) == -1)
			perror("send");
		if (send_data(client_map[socketfd], &throw_notif_packet) == -1)
			perror("send");
		disconnect_chat_both(socketfd, client_map[socketfd]);
	}
	else if(client_map[socketfd] == socketfd){

		Packet throw_packet;
		throw_packet.msgType = CONFIRM_M;

		strcpy(throw_packet.data, "Server has thrown you out of your chat channel");
		if (send_data(socketfd, &throw_packet) == -1)
			perror("send");
		disconnect_chat(socketfd, 1);
	}
}

// ends all connections and channels of clients on sockets
void end() {
	int c;
	for (c = 0; c < MAX_CLIENTS; c++) {
		if (client_map[c] > -2) {
			throwout(c);
			Packet end_packet;
			end_packet.msgType = CONFIRM_M;
			strcpy(end_packet.data, "Server has thrown you out of the chat queue");
			if (send_data(c, &end_packet) == -1)
				perror("send");
		}
		client_map[c] = -2;
		client_bytes[c] = 0;
		memset(&client_alias[c], 0, MAXCOMMANDSIZE*sizeof(client_alias[0]));
	}
	ready = 0;
	enable_stats = 0;
}

// Convert strings to all uppercase
void allCaps(char *command) {
	while(*command != '\0') {
		*command = toupper(*command);
		command++;
	}
}

// input/ouput handler function to be called on thread
void *io_handler(void *param, int socket_fd) {

	char command[MAXCOMMANDSIZE];	// For receiving commands from user
	int exitsignal;		// If user wants to end the application (Command: EXIT, value: 8)


	// Main process loop for client
	while(fgets(command, sizeof command, stdin)) {
		
		// Manual removal of newline character
		int len = strlen(command);
		if (len > 0 && command[len-1] == '\n') {
			command[len-1] = '\0';
		}
		allCaps(command);

		// parse the command, check if either single or double arg command
		int token_count = 0;
		int idx = 1;
		char* pch;

		char s[2][MAXCOMMANDSIZE];
		memset(s[0], "", MAXCOMMANDSIZE);
		memset(s[1], "", MAXCOMMANDSIZE);

		int isnul = strlen(command);
		if (isnul == 0)
			continue;

		pch = strtok(command, " ");
		strcpy(s[0], pch);
		while (pch != NULL)
	  	{
	  		pch = strtok(NULL, " ");
			if (pch != NULL) {
				strcpy(s[1], pch);
		    	idx++;
		    	if (idx > 1)
		    		break;
		    }
	    }

		if (idx == 1) {
			char* c1 = &s[0];
			if(!strcmp(c1, "EXIT")) {
				/* clean up */
				printf("Terminating server...\n");
				close(socket_fd);
				exit(0);
			}
			else if(!strcmp(c1, "START")){
				ready = 1;
				enable_stats = 1;
				if (abs_start == 0)
					abs_start = 1;
				else
					printf("Server ready: waiting for connections...\n");
			}
			else if (!strcmp(c1, "END")){
				printf ("Server terminating...\n");
				end();
				printf ("...done\n");
			}
			else {
				fprintf(stderr, "Unknown command: %s...\n", command);
			}
		}
		else if (idx == 2) {
			char* c1 = &s[0];
			char* c2 = &s[1];
			if(!strcmp(c1, "THROWOUT")) {
				int socket = atoi(c2);
				printf("Throwing out socket %d\n", socket);
				throwout(socket);
			}
			else {
				fprintf(stderr, "Unknown command: %s...\n", command);
			}
		}
		else {
			fprintf(stderr, "Unknown command: %s...\n", command);
		}
	}

	return NULL;
}

int main() {

	client1_connected = 0;
	client2_connected = 0;

	char empty_name[ALIASSIZE];
	reset_client_maps();
	reset_aliases();
	enable_stats = 0;
	abs_start = 0;

	fd_set master;	// master file descriptor list, holds all socket descriptors currently
					// - connected and the socket descriptor listening for new connections
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

	int listener, new_fd ,err_ret;
	struct addrinfo hints, *servinfo, *p;

	struct sockaddr_storage remoteaddr;	// Client socket address information
	socklen_t addrlen;	// Size of client's socket address

	int error_status;
	int yes = 1;
	char remoteIP[INET6_ADDRSTRLEN];

	FD_ZERO(&master);
    FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints);	// make sure the struct is empty
	hints.ai_family = AF_UNSPEC;		// don't care if IPv4 (AF_INET) or IPv6 (AF_INET6)
	hints.ai_socktype = SOCK_STREAM;	// use TCP stream sockets
	hints.ai_flags = AI_PASSIVE;		// fill in my IP for me
						
	ready = 0;
	// Returns the name of the computer this is running on
	char hostname[256];
	int gothostname = gethostname(hostname, 256);

	printf("My hostname: %s\n", hostname);
	 /* initiate interrupt handler for IO controlling */
 	pthread_t io_thread;
	printf("Starting admin interface...\n");
	if(pthread_create(&io_thread, NULL, io_handler, NULL) != 0) {
		err_ret = errno;
		fprintf(stderr, "pthread_create() failed...\n");
		return err_ret;
	}

	printf("Waiting for start command...\n");
	while (!ready);
				// -- tells getaddrinfo() to assign the address of my local host to the socket structures
	if ((error_status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) 
	{
		fprintf(stderr, "selectserver [getaddrinfo()]: %s\n", gai_strerror(error_status));
		exit(1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		// printf("p is %d with vals %d %d %d\n", p->ai_family, p->ai_socktype, p->ai_protocol);
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) 
		{
			close(listener);
			perror("server: bind");
			continue;
		}

		break;
	}
	
	// Free the linked list when all done with *servinfo
	freeaddrinfo(servinfo);

	// If *servinfo is empty, then fail to bind a socket to a port
	if (p == NULL) 
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	// Listen to incoming connections on the port, return -1 on error
	if (listen(listener, BACKLOG) == -1) 
	{
		perror("listen");
		exit(3);
	}

	// Add the listener to the master set
    FD_SET(listener, &master);

    // Keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

	//=================================================================================
	
	printf("Server ready: waiting for connections...\n");
	enable_stats = 1;

	// Main loop for multi-client chat server
	int i, j;	// Just iterator variables for this main loop
    while(1){
    	// Copy master descriptor list to temporary descriptor list
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        // Run through the existing connections looking for data to read
        // printf("read_fs is %d\n", read_fds);
        // printf("fdmax is %d\n", fdmax);

        for(i = 0; i <= fdmax; i++) {
        	// We got one - fd i is in the set read_fds
            if (FD_ISSET(i, &read_fds)){
                if (i == listener) {
                    // Handle new connections:
                    // Accept an incoming connection on the listening socket descriptor, listener
                    addrlen = sizeof remoteaddr;
                    new_fd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    // printf("i is %d and new_fd is %d\n", i, new_fd);

                    // If error occurs on accept(), it returns -1 and sets errno
                    if (new_fd == -1) 
                        perror("accept");
                    else{
                        FD_SET(new_fd, &master); // add to master set
                        if (new_fd > fdmax)
                            fdmax = new_fd;	// keep track of the max
                        
                        printf("selectserver: new connection from %s on socket %d\n",
								inet_ntop(remoteaddr.ss_family, 
										  get_in_addr((struct sockaddr*)&remoteaddr),
										  remoteIP, INET6_ADDRSTRLEN), new_fd);
                       	connect_client(new_fd);
                    }
                } 
                else{
            		// Handle data from a client
			        handle_client_adv(i, &master);
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END while(1)
	return 0;	
}

