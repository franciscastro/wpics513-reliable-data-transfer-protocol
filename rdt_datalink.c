/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Datalink file
*/

#include "config.h"
#include "rdt_datalink.h"

// Current transfer protocol in use
int transferProtocol;

// Pointer to buffer entries
BufferEntry * dlinkBuffer;

// Initialize the datalink and its variables
void datalinkInit(char * protocol) {

    // Transfer protocol is Go-back-N
    if (strcmp(protocol, "gbn") == 0) {
        transferProtocol = GOBACKN;

    } 
    // Transfer protocol is Selective-repeat
    else if (strcmp(protocol, "sr") == 0) {
        transferProtocol = SELREPEAT;
    }

    // Initialize the datalink temporary buffer
    dlinkBuffer = NULL;
}

// Add client packet to datalink buffer
void datalinkBuffer(int c_sockfd, Packet msg) {

    // Create a new buffer entry
    BufferEntry * newEntry = malloc(sizeof(newEntry));
    newEntry->next = NULL;
    newEntry->pkt = msg;
    
    // If the datalink buffer is empty
    if (dlinkBuffer == NULL) {
        dlinkBuffer = newEntry;
    }
    // If the datalink buffer is not empty
    else {

        // Temporary BufferEntry pointer to traverse the buffer
        BufferEntry * temp = dlinkBuffer;

        // Look for the end of the buffer linked list
        while(temp != NULL) {

            // Attach the client packet to the end of the buffer
            if (temp->next == NULL) {
                temp->next = newEntry;
            }
            temp = temp->next;
        }
    }
}

// Return 1 on success, 0 on fail
void datalinkFetch(Packet * buffer) {
    
    // Copy packet from temporary datalink buffer to actual buffer
    buffer = dlinkBuffer->pkt;

    // Point temporary pointer to current packet
    BufferEntry * temp = dlinkBuffer;

    // Move the datalink buffer pointer forward
    dlinkBuffer = dlinkBuffer->next;

    // Deallocate memory
    free(temp);
}

/*
void datalinkSend() {
    // If transfer protocol is Go-back-N
    if (transferProtocol == GOBACKN) {

        // Call Go-back-N, passing the CURRENT packet in the buffer
        gbnSend(c_sockfd, dlinkBuffer->pkt);

    }
    // If transfer protocol is Selective Repeat
    else if (transferProtocol == SELREPEAT) {

        // Call Selective repeat, passing the CURRENT packet in the buffer
        srSend(c_sockfd, dlinkBuffer->pkt);

    }
}
*/