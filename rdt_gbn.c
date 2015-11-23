/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Go-back-N file
*/

#include "config.h"
#include "rdt_datalink.h"

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
    
    s.info = buffer[frame_nr];      // Insert packet into frame
    s.seqNumber = frame_nr;         // Insert sequence number into frame

    s.nextAckExpected = (frameExpected + WINDOWSIZE) % (WINDOWSIZE + 1);     // Piggyback ack
    
    to_physical_layer(&s);      // Transmit the frame
    start_timer(frame_nr);      // Start the timer
}

// Go-back-N algorithm
void gbnSend() {

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
                inc(nextFrameToSend); 
                
                break;

            // Data or ACK frame has arrived
            case FRAME_ARRIVAL: 
                
                // Get incoming frame from physical layer
                from_physical_layer( &r ); 
                
                // Frames are accepted only in order
                if ( r.seqNumber == frameExpected ) {

                    datalinkTake( &r.pkt );     // Pass packet to datalink buffer
                    inc( frameExpected );         // Advance lower edge of receiver’s window

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

        if (nbuffered < WINDOWSIZE) { enable_upper_layer() };
        else { disable_upper_layer(); }
    }
}