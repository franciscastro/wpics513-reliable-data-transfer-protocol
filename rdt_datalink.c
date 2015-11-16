/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 15 Nov 2015

Datalink file
*/

#include "config.h"
#include "rdt_datalink.h"

int transferProtocol;

void datalinkInit(char * protocol) {

    // Transfer protocol is Go-back-N
    if (strcmp(protocol, "gbn") == 0) {
        transferProtocol = GOBACKN;

    } 
    // Transfer protocol is Selective-repeat
    else if (strcmp(protocol, "sr") == 0) {
        transferProtocol = SELREPEAT;
    }
}

// Return 1 on success, 0 on fail
int datalinkSend(int c_sockfd, Packet msg) {
    
    if (transferProtocol == GOBACKN) {
        // gbnSend(c_sockfd, AppMessage msg)
    }
    else if (transferProtocol == SELREPEAT) {
        // srSend(c_sockfd, AppMessage msg)
    }

    return 1;
}

// Return true if a <= b < c circularly; false otherwise.
static boolean between(int a, int b, int c) {
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return(true);
    else
        return(false);
}

// Construct and send a data frame
static void send_data(int frame_nr, int frame_expected, Packet buffer[]) {
    
    Frame s;    // Temporary Frame
    
    s.info = buffer[frame_nr];      // Insert packet into frame
    s.seqNumber = frame_nr;         // Insert sequence number into frame

    s.nextAckExpected = (frame_expected + WINDOWSIZE) % (WINDOWSIZE + 1);     // Piggyback ack
    
    to_physical_layer(&s);      // Transmit the frame
    start_timer(frame_nr);      // Start the timer
}

void gbnSend(int c_sockfd, Packet msg) {

    int next_frame_to_send = 0;  // WINDOWSIZE > 1; used for outbound stream; initially 0
    int ack_expected = 0;        // Oldest unACKED frame; initially 0
    int frame_expected = 0;      // Next frame_expected on inbound stream; initially 0
    
    Frame r;                        // Temporary frame variable
    Packet buffer[WINDOWSIZE + 1];  // buffers for the outbound stream
    
    int nbuffered = 0;   // Number of output buffers currently in use; initially 0
    
    enable_network_layer();     // allow network layer ready events

    EventType event;
    
    while (1) {

        // 4 possibilities: see event type above
        wait_for_event(&event); 
            
        switch(event) {
            
            // Network layer has a packet to send
            case NETWORK_LAYER_READY:       
                
                // Accept, save, and transmit a new frame
                from_network_layer(&buffer[next_frame_to_send]); // Fetch new packet
                
                // Expand the sender’s window
                nbuffered = nbuffered + 1; 
                
                // Transmit the frame
                send_data(next_frame_to_send, frame_expected, buffer);
                
                // advance sender’s upper window edge
                inc(next_frame_to_send); 
                
                break;

            // Data or ACK frame has arrived
            case FRAME_ARRIVAL: 
                
                // Get incoming frame from physical layer
                from_physical_layer(&r); 
                
                // Frames are accepted only in order.
                if (r.seq == frame_expected) {
                    to_network_layer(&r.info);  // Pass packet to network layer
                    inc(frame_expected);        // Advance lower edge of receiver’s window
                }

                // Ack n implies n − 1, n − 2, etc. Check for this.
                while (between(ack_expected, r.ack, next_frame_to_send)) {
                    
                    // Handle piggybacked ack
                    nbuffered = nbuffered − 1;  // one frame fewer buffered
                    stop_timer(ack_expected);   // frame arrived intact; stop timer
                    inc(ack_expected);          // contract sender’s window
                }
                break;

            // Ignore bad frames
            case CKSUM_ERR: break; 
        
            // trouble; retransmit all outstanding frames
            case TIMEOUT: 

                // Start retransmitting here
                next_frame_to_send = ack_expected;

                int i;  // Used to index into the buffer array
                
                for (i = 1; i <= nbuffered; i++) {
                    send_data(next_frame_to_send, frame_expected, buffer);  /* resend frame */
                    inc(next_frame_to_send);    /* prepare to send the next one */
                }
        }

        if (nbuffered < WINDOWSIZE) { enable_network_layer() };
        else { disable_network_layer(); }
    }
}