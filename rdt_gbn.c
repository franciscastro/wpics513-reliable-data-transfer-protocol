/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Go-back-N file
*/

#include "config.h"
#include "rdt_gbn.h"

boolean upperLayerEnabled = false;      // Flag to determine if datalink can pass down a packet
boolean frameArrived = false;           // Flag to determine if frame from physical layer has arrived

pthread_t gbnThread;            // Thread for Go-Back-N passer to physical layer
pthread_t gbnThreadReceiver;    // Thread for Go-Back-N receiver from physical layer

void gbnInit() {
    if (pthread_create(&gbnThread, NULL, gbn, NULL)) {
        fprintf(stderr, "Error creating GBN thread.");
        exit(1);
    }
}

// Return true if a <= b < c circularly; false otherwise.
static boolean between(int a, int b, int c) {
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return(true);
    else
        return(false);
}

// Construct and send a data frame
static void sendData(int frame_nr, int frameExpected, Packet buffer[]) {
    
    Frame s;    // Temporary Frame
    
    s.pkt = buffer[frame_nr];      // Insert packet into frame
    s.seqNumber = frame_nr;         // Insert sequence number into frame

    s.nextAckExpected = (frameExpected + WINDOWSIZE) % (WINDOWSIZE + 1);     // Piggyback ack
    
    to_physical_layer( &s );      // Transmit the frame
    start_timer( frame_nr );      // Start the timer
}

// Wait for an event to happen; return its type in event
void wait_for_event(EventType * event) {

    // CASE 1: There's data in outgoing datalink buffer AND datalink is allowed to pass down packets to protocol
    if ( upperLayerEnabled == true ) {
        (*event) = UPPER_LAYER_READY;
        return;
    }
    // CASE 2: There's data incoming from physical layer
    // CASE 3: There's a checksum error
    else if ( frameArrived == true ) {
        // Check if frame is corrupt
        if ( checkCorrupt() == 1 ) {
            (*event) = CKSUM_ERR;
        }
        // Frame is not corrupt
        else {
            (*event) = FRAME_ARRIVAL;
        }
    }
    // CASE 4: There's a timeout
    else if ( 1 ) {
        (*event) = TIMEOUT;
    }
}

// Allow GBN to get packets from outgoing datalink buffer
void enable_upper_layer() {

    if ( fromClient != NULL ) {
        upperLayerEnabled = true;
    }
}

// Forbid GBN from getting packets from outgoing datalink buffer
void disable_upper_layer() {

    upperLayerEnabled = false;
}

// Change frameArrived to true:
// Triggered by physical layer
void frameArrivedSignal(boolean val) {

    frameArrived = val;
}

// Go-back-N algorithm
void *gbn(void *param) {

    int nextFrameToSend = 0;    // WINDOWSIZE > 1; used for outbound stream; initially 0
    int ackExpected = 0;        // Oldest unACKed frame; initially 0
    int frameExpected = 0;      // Next frameExpected on inbound stream; initially 0
    
    Frame r;                        // Temporary frame variable
    Packet buffer[WINDOWSIZE + 1];  // buffers for the outbound stream
    
    int nbuffered = 0;   // Number of output buffers currently in use; initially 0
    
    enable_upper_layer();     // allow upper layer ready events

    EventType event;
    
    while (1) {

        // 4 possibilities: see event types in config.h
        wait_for_event(&event); 
            
        switch(event) {
            
            // Upper layer has a packet to send
            case UPPER_LAYER_READY:       
                
                // Accept, save, and transmit a new frame
                datalinkFetch( &buffer[nextFrameToSend] ); // Fetch new packet
                
                // Expand the sender's window
                nbuffered = nbuffered + 1; 
                
                // Transmit the frame
                sendData( nextFrameToSend, frameExpected, buffer );
                
                // Advance sender's upper window edge
                inc( nextFrameToSend ); 
                
                break;

            // Data or ACK frame has arrived
            case FRAME_ARRIVAL: 
                
                // Get incoming frame from physical layer
                from_physical_layer( &r ); 
                
                // Frames are accepted only in order
                if ( r.seqNumber == frameExpected ) {

                    datalinkTake( &r.pkt );     // Pass packet to datalink buffer
                    inc( frameExpected );       // Advance lower edge of receiver’s window

                }

                // Ack n implies n − 1, n − 2, etc. Check for this.
                while ( between( ackExpected, r.nextAckExpected, nextFrameToSend ) ) {
                    
                    // Handle piggybacked ack
                    nbuffered = nbuffered − 1;  // one frame fewer buffered
                    stop_timer( ackExpected );   // frame arrived intact; stop timer
                    inc( ackExpected );          // contract sender’s window
                }

                break;

            // Ignore bad frames
            case CKSUM_ERR: break; 
        
            // Trouble; retransmit all outstanding frames
            case TIMEOUT: 

                // Start retransmitting here
                nextFrameToSend = ackExpected;

                int i;  // Used to index into the buffer array
                
                for (i = 1; i <= nbuffered; i++) {
                    
                    sendData( nextFrameToSend, frameExpected, buffer );  // Resend frame
                    
                    inc( nextFrameToSend );    // Prepare to send the next one

                }
        }

        // If the number of currently buffered packets is less than WINDOWSIZE, enable receiving packets from datalink
        if ( nbuffered < WINDOWSIZE ) { enable_upper_layer(); }
        // If the number of currently buffered packets is WINDOWSIZE, can't receive packets from datalink
        else { disable_upper_layer(); }
    }
}