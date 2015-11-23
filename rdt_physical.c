/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Physical layer file
*/

#include "config.h"
#include "rdt_physical.h"

int CORRUPTRATE;		// % of packets corrupted
int DROPRATE;			// % of packets to be dropped
int transferProtocol;	// Current transfer protocol in use
time_t t;				// For seeding random generator

void physicalInit(char * protocol, int corruptRate, int dropRate) {
	
	// Set corrupt and drop rates
	CORRUPTRATE = corruptRate;
	DROPRATE = dropRate;

	// Intialize random number generator
   srand((unsigned) time(&t));

	// Transfer protocol is Go-back-N
    if (strcmp(protocol, "gbn") == 0) {
        transferProtocol = GOBACKN;

    } 
    // Transfer protocol is Selective-repeat
    else if (strcmp(protocol, "sr") == 0) {
        transferProtocol = SELREPEAT;
    }

    // Start physical layer thread for waiting for data arrival here
}

void to_physical_layer(Frame * s) {

	Frame toSend = s;

	if ( ((rand() % 100) + 1) < CORRUPTRATE ) {
		// Corrupt frame here
	}

	if ( ((rand() % 100) + 1) < DROPRATE ) {
		// Drop packet here
		return;
	}

	int n = send(sockfd, toSend, bytesleft, 0);
}

// Thread function waiting for data to arrive
void physicalArrived() {

	Frame msgrecvd;		// Frame received
	int recvd;			// Number of bytes read into the buffer

	while (1) {

		recvd = recv(sockfd, (void *)&msgrecvd, sizeof(Frame), 0);

		if (!recvd) {
			fprintf(stderr, "Server connection lost. \n");
			isConnected = 0; 
			close(sockfd); 
			break;
		}
	}

}