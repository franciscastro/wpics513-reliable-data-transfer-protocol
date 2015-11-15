
#include "config.h"
#include "msg.h"
#include "client_lib.h"

// Remove whitespace
char* removeSpace(char *s) {

    size_t size = strlen(s);	// Check for valid length
    if (!size) { return s; }

    char *end = s + size - 1;
    while (end >= s && isspace(*end)) { end--; }

    *(end + 1) = '\0';	// Terminate this string

    while (*s && isspace(*s)) { s++; }
    
    return s;
}

// Get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {

	// sockaddr is IPv4
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	// else, sockaddr is IPv6
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Convert strings to all uppercase
void allCaps(char *command) {
	while(*command != '\0') {
		*command = toupper(*command);
		command++;
	}
}

// Connect to server: return sockfd on success, -1 on error
int connectToServer() {

	// Socket file descriptor for communicating to remote host
	int sockfd;

	// Holds the error state when getaddrinfo() is called
	int error_status;

	// For prepping sockaddrs later: 
	// - hints points to an addrinfo to be filled with information
	// - *servinfo points to a linked list of struct addrinfo
	// - *p serves as a temporary pointer to hold *servinfo's data later
	struct addrinfo hints, *servinfo, *p;

	// [ Load up address structs with getaddrinfo() ]
	//=================================================================================

	// Setup values in hints
	memset(&hints, 0, sizeof hints);	// make sure the struct is empty
	hints.ai_family = AF_UNSPEC;		// don't care if IPv4 (AF_INET) or IPv6 (AF_INET6)
	hints.ai_socktype = SOCK_STREAM;	// use TCP stream sockets
	
	// Call getaddrinfo() to setup the structures in hints
	// - error: getaddrinfo() returns non-zero
	// - success: servinfo will point to a linked list of struct addrinfo,
	//			  each of which contains a struct sockaddr to be used later
	if ((error_status = getaddrinfo(HOSTNAME, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error_status));
		return -1;
	}

	//=================================================================================

	// [ Make a socket, connect() to destination ]
	//=================================================================================

	// Loop through all the results in *servinfo and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {

		// Make a socket
		// - assign a socket descriptor to sockfd on success, -1 on error
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("Client socket");
			continue;
		}

		// Connect to remote host in the destination port and IP address
		// - return: -1 on error and set errno to the error's value
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Client connect");
			continue;
		}

		break;
	}

	//=================================================================================

	// Free the linked list when all done with servinfo
	freeaddrinfo(servinfo);

	// If servinfo is empty, then fail to connect
	if (p == NULL) {
		fprintf(stderr, "Client: Failed to connect\n");
		return -1;
	}

	// Prompt client that it is now waiting to recv() on pthread
	fprintf(stdout, "Now waiting for data on [%i]...\n", sockfd);

	return sockfd;
}

// Show client commands
void help() {
	printf("Client commands:\n");
	printf("%-10s - connect to the server.\n", CONNECT);
	printf("%-10s - request for a chat partner.\n", CHAT);
	printf("%-10s - quit the current chat channel.\n", QUIT);
	printf("%-10s - send a file to chat partner.\n", TRANSFER);
	printf("%-10s - print this help information.\n", HELP);
	printf("%-10s - send a message to chat partner.\n", MESSAGE);
	printf("%-10s - terminate and exit the program.\n", EXIT);
	printf("%-10s - check with the server if you are in a chat queue.\n", CONFIRM);
}
