/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Selective Repeat file
*/

#include "config.h"
#include "rdt_sr.h"

#define NR_BUFS ((WINDOWSIZE + 1)/2)

boolean no_nak = true;

// Return true if a <= b < c circularly; false otherwise.
static boolean between(int a, int b, int c) {
    if (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)))
        return(true);
    else
        return(false);
}

static void send_frame(FrameType ft, int frame_nr, int frame_expected, Packet buffer[]) {
	
	// Construct and send data, ACK, or NAK frame
	Frame s;
	s.type = ft; 
	
	if (ft == DATA_F) {
		s.pkt = buffer[frame_nr % NR_BUFS];
	}
	
	s.seqNumber = frame_nr;
	s.nextAckExpected = (frame_expected + WINDOWSIZE) % (WINDOWSIZE + 1);
	
	//if (ft == NAK) no nak = false; /* one nak per frame, please */
	
	to_physical_layer(&s); // Transmit the frame
	
	if (fk == DATA_F) {
		start_timer(frame_nr % NR_BUFS);
	}
	
	stop_ack_timer();
}

void srSend() {

	int ack_expected = 0; 			// Lower edge of sender’s window; next ack expected on the inbound stream
	int next_frame_to_send = 0; 	// Upper edge of sender’s window + 1; number of next outgoing frame
	int frame_expected = 0;			// Lower edge of receiver’s window
	int too_far = NR_BUFS; 			// Upper edge of receiver’s window + 1
	
	Frame r; 						// Temporary frame variable
		
	Packet out_buf[NR_BUFS]; 		// Buffers for the outbound stream
	Packet in_buf[NR_BUFS]; 		// Buffers for the inbound stream
	
	boolean arrived[NR_BUFS]; 		// Inbound bit map
	
	int nbuffered = 0; 				// How many output buffers currently used

	enable_upper_layer(); 		// initialize

	int i;
	for (i = 0; i < NR_BUFS; i++)  {
		arrived[i] = false;
	}

	EventType event;

	while (1) {
		
		wait_for_event(&event); /* five possibilities: see event type above */
		
		switch(event) {

			// Accept, save, and transmit a new frame
			case UPPER_LAYER_READY:

				// Expand the window
				nbuffered = nbuffered + 1;
				
				// Fetch new packet
				datalinkFetch( &out_buf[next_frame_to_send % NR_BUFS] ); 
				
				// Transmit the frame
				send_frame( data, next_frame_to_send, frame_expected, out_buf );
				
				// Advance upper window edge
				inc( next_frame_to_send );
				
				break;
			
			// Data or control frame has arrived
			case FRAME_ARRIVAL:

				// Fetch incoming frame from physical layer
				from_physical_layer(&r);
				
				if (r.type == DATA_F) {

					// Undamaged frame has arrived
					if ((r.seqNumber != frame_expected) && no_nak) {
						send_frame(nak, 0, frame_expected, out_buf); 
					}

					else {
						start_ack_timer();
					}
					
					if ( between( frame expected, r.seqNumber, too_far ) && ( arrived[r.seqNumber % NR_BUFS] == false )) {
					
						// Frames may be accepted in any order
						arrived[r.seqNumber % NR_BUFS] = true; 		// Mark buffer as full
						in_buf[r.seqNumber % NR_BUFS] = r.info; 	// Insert data into buffer
						
						while (arrived[frame_expected % NR_BUFS]) {
							
							// Pass frames and advance window
							to_network_layer(&in_buf[frame_expected % NR_BUFS]);
							
							no_nak = true;
							arrived[frame_expected % NR_BUFS] = false;
							
							inc( frame_expected ); 	// Advance lower edge of receiver’s window
							inc( too_far ); 		// Advance upper edge of receiver’s window
							
							start_ack_timer();	// to see if a separate ack is needed
						}
					}
				}

				if ( (r.type==nak) && between( ack_expected, (r.nextAckExpected + 1) % (WINDOWSIZE + 1), next_frame_to_send ) ) {
					send_frame( data, (r.nextAckExpected + 1) % (WINDOWSIZE + 1), frame_expected, out_buf );
				}
				
				while ( between( ack_expected, r.nextAckExpected, next_frame_to_send ) ) {
					
					// Handle piggybacked ack
					nbuffered = nbuffered − 1;
					
					// Frame arrived intact
					stop_timer( ack_expected % NR_BUFS );
					
					// Advance lower edge of sender’s window
					inc( ack_expected ); 
				}
				break;
			
			case CKSUM_ERR:

				if (no_nak) {
					send_frame(nak, 0, frame_expected, out_buf);	// Damaged frame
				}
				
				break;
			
			case TIMEOUT:
				
				// We timed out
				send_frame( data, oldest_frame, frame_expected, out_buf );
				
				break;
			
			case ACK_TIMEOUT:

				// ACK timer expired; send ACK
				send_frame( ack, 0, frame_expected, out_buf );
		}

		if (nbuffered < NR_BUFS) { enable_upper_layer(); } 
		else { disable_upper_layer(); }
	}
}