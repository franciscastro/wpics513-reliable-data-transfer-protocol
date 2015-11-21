/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 15 Nov 2015

Header file for datalink definitions
*/

#ifndef RDT_DATALINK_H_
#define RDT_DATALINK_H_

// Structure definition for a temporary buffer space to store Packets received from the client application
typedef struct BufferEntry {
    BufferEntry * next;
    Packet pkt;
} BufferEntry;

// Initialize the datalink and its variables
void datalinkInit(char * protocol);

// Add client packet to datalink buffer
void datalinkBuffer(int c_sockfd, Packet * msg);

// Fetch a packet from the datalink temporary buffer
void datalinkFetch();

#endif