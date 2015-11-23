/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Datalink file
*/

#include "config.h"
#include "rdt_datalink.h"

int transferProtocol;       // Current transfer protocol in use

BufferEntry * fromClient;   // Pointer to outgoing buffer for storing packets from client
BufferEntry * forClient;    // Pointer to incoming buffer for storing packets received from physical layer

pthread_t gbnThread;            // Thread for Go-Back-N passer to physical layer
pthread_t gbnThreadReceiver;    // Thread for Go-Back-N receiver from physical layer

pthread_t srThread;             // Thread for Selective-repeat passer to physical layer
pthread_t srThreadReceiver;     // Thread for Selective-repeat receiver from physical layer

// Initialize the datalink and its variables
void datalinkInit(char * protocol) {

    // Transfer protocol is Go-back-N
    if (strcmp(protocol, "gbn") == 0) {
        transferProtocol = GOBACKN;

        // Start thread for GBN
        if (pthread_create(&gbnThread, NULL, &gbn, NULL)) {
            fprintf(stderr, "Error creating GBN thread.");
            exit(1);
        }
        printf("\nGBN thread created.");
    } 
    // Transfer protocol is Selective-repeat
    /*else if (strcmp(protocol, "sr") == 0) {
        transferProtocol = SELREPEAT;

        // Start thread for SELREPEAT
        if (pthread_create(&srThread, NULL, &sr, NULL)) {
            fprintf(stderr, "Error creating SELREPEAT thread.");
            exit(1);
        }
        printf("\nSELREPEAT thread created.");
    }*/

    // Initialize the datalink buffers
    fromClient = NULL;
    forClient = NULL;
}

// Receive packet from client:
// Add client packet to datalink outgoing buffer
void datalinkSend(int c_sockfd, Packet * msg) {

    // Create a new buffer entry
    BufferEntry * newEntry = malloc(sizeof(newEntry));
    newEntry->next = NULL;
    newEntry->pkt = (*msg);
    
    // If the outgoing datalink buffer is empty
    if ( fromClient == NULL ) {
        fromClient = newEntry;
    }
    // If the outgoing datalink buffer is not empty
    else {

        // Temporary BufferEntry pointer to traverse the buffer
        BufferEntry * temp = fromClient;

        // Look for the end of the buffer linked list
        while ( temp != NULL ) {

            // Attach the client packet to the end of the buffer
            if ( temp->next == NULL ) {
                temp->next = newEntry;
            }

            // Move to next BufferEntry
            temp = temp->next;
        }
    }
}

// Receive packet from GBN/SELREPEAT:
// Add GBN/SELREPEAT packet to datalink incoming buffer
void datalinkTake(Packet * pktReceived) {

    // Create new buffer entry
    BufferEntry * newEntry = malloc(sizeof(newEntry));
    newEntry->next = NULL;
    newEntry->pkt = pktReceived;

    // If the incoming datalink buffer is empty
    if ( forClient == NULL ) {
        forClient = newEntry;
    }
    // If the incoming datalink buffer is not empty
    else {

        // Temporary BufferEntry pointer to traverse the buffer
        BufferEntry * temp = forClient;

        // Look for the end of the buffer linked list
        while ( temp != NULL ) {

            // Attach the incoming packet to the end of the buffer
            if ( temp->next == NULL ) {
                temp->next = newEntry;
            }
            temp = temp->next;
        }
    }
}

// Pass packet up to client: 
// Triggered by client
void datalinkReceive(Packet * clientReceiver) {

    // Copy packet from datalink incoming buffer
    clientReceiver = forClient->pkt;

    // Point temporary pointer to current packet
    BufferEntry * temp = forClient;

    // Move datalink buffer pointer forward
    forClient = forClient->next;

    // Deallocate memory
    free(temp);
}

// Pass packet to GBN/SELREPEAT buffer:
// Triggered by GBN/SELREPEAT
void datalinkFetch(Packet * buffer) {
    
    // Copy packet from datalink outgoing buffer to GBN buffer
    buffer = fromClient->pkt;

    // Point temporary pointer to current packet
    BufferEntry * temp = fromClient;

    // Move the datalink buffer pointer forward
    fromClient = fromClient->next;

    // Deallocate memory
    free(temp);
}

/*
void datalinkTransport() {
    // If transfer protocol is Go-back-N
    if (transferProtocol == GOBACKN) {

        // Call Go-back-N, passing the CURRENT packet in the buffer
        gbnSend(c_sockfd, fromClient->pkt);

    }
    // If transfer protocol is Selective Repeat
    else if (transferProtocol == SELREPEAT) {

        // Call Selective repeat, passing the CURRENT packet in the buffer
        srSend(c_sockfd, fromClient->pkt);

    }
}
*/