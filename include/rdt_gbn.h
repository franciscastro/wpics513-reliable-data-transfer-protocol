/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Header file for Go-back-N definitions
*/

#ifndef RDT_GBN_H_
#define RDT_GBN_H_

// Wait for an event to happen; return its type in event
void wait_for_event(event_type *event);	

// Fetch a packet from the network layer for transmission on the channel
// void from_upper_layer(Packet *p);		

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

// Allow the upper layer to cause an upper layer ready event
void enable_upper_layer(void);		

// Forbid the upper layer from causing an upper layer ready event
void disable_upper_layer(void);		

// Macro inc is expanded in-line: increment k circularly
#define inc(k) if (k < MAX_SEQ) k = k + 1; else k = 0

#endif