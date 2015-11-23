/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Header file for physical layer definitions
*/

#ifndef RDT_PHYSICAL_H_
#define RDT_PHYSICAL_H_

// Structure definition for a temporary buffer space to store Frames received
typedef struct FrameBuffer {
    FrameBuffer * next;
    Frame frm;
} FrameBuffer;

void physicalInit(char * protocol, int corruptRate, int dropRate);

// Get incoming frame from physical layer
void from_physical_layer(Frame *f);

// Send data out
void to_physical_layer(Frame * s);

// Thread function waiting for data to arrive
void *physicalArrived(void *param)

#endif