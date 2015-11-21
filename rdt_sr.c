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
		
	Packet out_buf[NR BUFS]; 		// Buffers for the outbound stream
	Packet in_buf[NR BUFS]; 		// Buffers for the inbound stream
	
	boolean arrived[NR BUFS]; 		// inbound bit map
	
	int nbuffered = 0; 				// how many output buffers currently used
	
	EventType event;

	enable_network_layer(); 		// initialize

	int i;
	for (i = 0; i < NR BUFS; i++)  {
		arrived[i] = false;
	}

	while (true) {
		
		wait_for_event(&event); /* five possibilities: see event type above */
		
		switch(event) {

			// Accept, save, and transmit a new frame
			case UPPER_LAYER_READY:

				// Expand the window
				nbuffered = nbuffered + 1;
				
				// Fetch new packet
				datalinkFetch( &out_buf[next_frame_to_send % NR_BUFS] ); 
				
				// transmit the frame
				send_frame(data, next frame to send, frame expected, out buf);
				
				// advance upper window edge
				inc(next frame to send);
				
				break;
			
			// Data or control frame has arrived
			case FRAME_ARRIVAL:

				// fetch incoming frame from physical layer
				from_physical_layer(&r);
				
				if (r.type == DATA_F) {

					/* An undamaged frame has arrived. */
					if ((r.seqNumber != frame_expected) && no_nak) {
						send_frame(nak, 0, frame_expected, out_buf); 
					}

					else {
						start_ack_timer();
					}
					
					if (between(frame expected,r.seq,too far) && (arrived[r.seq%NR BUFS]==false)) {
					
					/* Frames may be accepted in any order. */
					arrived[r.seqNumber % NR_BUFS] = true; 		// Mark buffer as full
					in_buf[r.seqNumber % NR_BUFS] = r.info; 	// insert data into buffer
					
					while (arrived[frame expected % NR BUFS]) {
						
						// Pass frames and advance window
						to_network_layer(&in_buf[frame_expected % NR_BUFS]);
						no_nak = true;
						arrived[frame expected % NR BUFS] = false;
						inc(frame expected); /* advance lower edge of receiver’s window */
						inc(too far); /* advance upper edge of receiver’s window */
						start ack timer(); /* to see if a separate ack is needed */
					}
					}
				}

				if ( (r.type==nak) && between( ack_expected, (r.nextAckExpected + 1) % (WINDOWSIZE + 1), next_frame_to_send ) ) {
					send_frame(data, (r.nextAckExpected + 1) % (WINDOWSIZE + 1), frame_expected, out_buf);
				}
				
				while (between(ack expected, r.ack, next frame to send)) {
					
					// Handle piggybacked ack
					nbuffered = nbuffered − 1;
					
					// Frame arrived intact
					stop_timer(ack_expected % NR_BUFS);
					
					// Advance lower edge of sender’s window
					inc(ack expected); 
				}
				break;
			
			case CKSUM_ERR:

				if (no_nak) {
					send_frame(nak, 0, frame_expected, out_buf); /* damaged frame */
				}
				
				break;
			
			case TIMEOUT:
				
				// We timed out
				send_frame( data, oldest frame, frame_expected, out_buf );
				
				break;
			
			case ACK_TIMEOUT:
				send_frame( ack, 0, frame_expected, out_buf ); /* ack timer expired; send ack */
		}

		if (nbuffered < NR BUFS) { enable_network_layer(); } 
		else { disable_network_layer(); }
	}
}