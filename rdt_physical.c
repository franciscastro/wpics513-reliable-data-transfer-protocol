/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Physical layer file
*/

#include "config.h"
#include "rdt_physical.h"

int CORRUPTRATE;			// % of packets corrupted
int DROPRATE;				// % of packets to be dropped
time_t t;					// For seeding random generator
pthread_t physicalWait;		// Physical layer thread to wait for data
FrameBuffer fbuff;			// Frame buffer for arrived frames

// Initialize physical layer
void physicalInit(char * protocol, int corruptRate, int dropRate) {
	
	// Set corrupt and drop rates
	CORRUPTRATE = corruptRate;
	DROPRATE = dropRate;

	// Intialize random number generator
   	srand((unsigned) time(&t));

   	// Start physical layer thread for waiting for data arrival here
    if (pthread_create(&physicalWait, NULL, &physicalArrived, NULL)) {
        fprintf(stderr, "Error creating physical layer thread.");
        exit(1);
    }
    printf("\nPhysical layer thread created.");

    // Initialize FrameBuffer
    fbuff = NULL;
}

// Get incoming frame from physical layer
void from_physical_layer(Frame *f) {

	// Copy packet from Frame buffer to protocol variable
    f = fbuff->frm;

    // Point temporary pointer to current FrameBuffer entry
    FrameBuffer * temp = fbuff;

    // Move the datalink buffer pointer forward
    fbuff = fbuff->next;

    // Deallocate memory
    free(temp);
}

// Send data out
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
void *physicalArrived(void *param) {

	Frame msgrecvd;		// Frame received
	int recvd;			// Number of bytes read into the buffer

	while (1) {

		recvd = recv(sockfd, (void *)&msgrecvd, sizeof(Frame), 0);

		if ( !recvd ) {
			fprintf(stderr, "Server connection lost. \n");
			isConnected = 0; 
			close(sockfd); 
			break;
		}

		if ( recvd > 0 ) {
			frameArrivedSignal( true );

			// Create a new buffer entry
		    FrameBuffer * newEntry = malloc(sizeof(newEntry));
		    newEntry->next = NULL;
		    newEntry->frm = msgrecvd;
		    
		    // If the Frame buffer is empty
		    if ( fbuff == NULL ) {
		        fbuff = newEntry;
		    }
		    // If the Frame buffer is not empty
		    else {

		        // Temporary FrameBuffer pointer to traverse the buffer
		        FrameBuffer * temp = fbuff;

		        // Look for the end of the buffer linked list
		        while ( temp != NULL ) {

		            // Attach the frame to the end of the buffer
		            if ( temp->next == NULL ) {
		                temp->next = newEntry;
		            }

		            // Move to next BufferEntry
		            temp = temp->next;
		        }
		    }
		}
	}

}