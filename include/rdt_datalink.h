/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 15 Nov 2015

Header file for datalink definitions
*/

#ifndef RDT_DATALINK_H_
#define RDT_DATALINK_H_

void datalinkInit(char * protocol);

int datalinkSend(int c_sockfd, AppMessage msg);

// Wait for an event to happen; return its type in event
void wait_for_event(event_type *event);	

// Fetch a packet from the network layer for transmission on the channel
void from_network_layer(Packet *p);		

// Deliver information from an inbound frame to the network layer
void to_network_layer(packet *p);		

// Go get an inbound frame from the physical layer and copy it to r
void from_physical_layer(Frame *r);		

// Pass the frame to the physical layer for transmission
void to_physical_layer(Frame *s);		

// Start the clock running and enable the timeout event
void start_timer(seq_nr k);				

// Stop the clock and disable the timeout event
void stop_timer(seq_nr k);				

// Start an auxiliary timer and enable the ack timeout event
void start_ack_timer(void);				

// Stop the auxiliary timer and disable the ack timeout event
void stop_ack_timer(void);				

// Allow the network layer to cause a network layer ready event
void enable_network_layer(void);		

// Forbid the network layer from causing a network layer ready event
void disable_network_layer(void);		

// Macro inc is expanded in-line: increment k circularly
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0

#endif