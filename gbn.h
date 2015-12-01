/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Go-Back-N ARQ file.
*/

#ifndef GBN_H_
#define GBN_H_

#include "config.h";
#include "datalink.h"
#include <time.h>

int gbn_wndw_sz = 10;
double gbn_max_time = 0.5;

/*
	GBN struct used to store information needed to perform ARQ
	Required because server side needs multiple different GBN
		values (one for each socket/client)
*/

typedef struct GBNThreadData {
	pthread_t thread_ID;		// This thread's pointer
} GBNThreadData;

typedef struct GBN_instance {

    double time_elpsd;
    double time_max;

    int sockfd;
    int last_ack_rcvd;
	int is_srvr;
	int is_sndr;
	int is_timed;
    int wndw_sz;
    int base;
    int nxt_sq_nm;
    int expt_sq_nm;
    int pkt_bfr_sz;
    int frame_bfr_sz;

    clock_t start, diff;

    BufferEntry * pkt_head;
    BufferEntry * pkt_tail;

    FrameBufferEntry * frame_head;
    FrameBufferEntry * frame_tail;

    // variables for linked list implementation
    struct GBN_instance * next;
    struct GBN_instance * prev;

    Packet snd_pkt;
    Frame snd_frame;
} GBN_instance;

void Destroy_GBN_instance(GBN_instance * gbn_inst) {
	// fix this later -> must null and free all members of the linked list
	gbn_inst->pkt_head = NULL;
	gbn_inst->pkt_tail = NULL;

	gbn_inst->frame_head = NULL;
	gbn_inst->frame_tail = NULL;

	gbn_inst->next = NULL;
	gbn_inst->prev = NULL;

	free(gbn_inst);
}

void gbn_init(int sockfd, int is_sndr, int is_srvr, int wndw_sz, double time_max, GBN_instance * gbn_inst) {
	srand((unsigned) time(&t));

	gbn_inst->sockfd		= sockfd; // which socket I'm listening to
	gbn_inst->is_sndr 		= is_sndr;
	gbn_inst->is_srvr 		= is_srvr;
	gbn_inst->time_max 		= time_max;
	gbn_inst->wndw_sz 		= wndw_sz;
	gbn_inst->time_elpsd 	= 0.0;
	gbn_inst->is_timed		= 0;
	gbn_inst->last_ack_rcvd	= 0;
	gbn_inst->frame_bfr_sz 	= 0;
	gbn_inst->pkt_bfr_sz	= 0;
	gbn_inst->base 			= 0;
	gbn_inst->nxt_sq_nm 	= 0;
	gbn_inst->frame_head 	= NULL;
	gbn_inst->frame_tail 	= NULL;
	gbn_inst->pkt_head 		= NULL;
	gbn_inst->pkt_tail 		= NULL;

	if (is_sndr) {
		gbn_inst->base = 1;
		gbn_inst->nxt_sq_nm = 1;	
	}
	else {
		gbn_inst->expt_sq_nm = 1;
		gbn_inst->snd_pkt.sockfd = sockfd;
		gbn_inst->snd_frame.pkt = gbn_inst->snd_pkt;
		gbn_inst->snd_frame.type = ACK_F;
		gbn_inst->snd_frame.seqNumber = 0;

		char * checksum = compute_checksum(gbn_inst->snd_frame);
		int c;
		for (c = 0; c < CHAR_BIT; ++c)
			gbn_inst->snd_frame.checksum[c] = checksum[c];
		// strcpy(gbn_inst->snd_frame.checksum, compute_checksum(gbn_inst->snd_frame));
	}
}

/*
	Called by physical layer to push a FrameBufferEntry (containing the received frame) into its
	corresponding FrameBuffer (determined by GBN type [server/client] and socket which the frame 
	came from)

	Have to hadle ACK frames differently (don't store them into packet rcvr)
*/
void gbn_rdt_rcv(GBN_instance * gbn_inst, Frame rcv_frame, int sockfd) {
	FrameBufferEntry * fbe = malloc(sizeof(FrameBufferEntry));
	fbe->frame = rcv_frame;
	fbe->next = NULL;

	if (rcv_frame.type == DATA_F) {
		if (!is_in_frame_data_rcv_buffer(gbn_inst->is_srvr, sockfd, fbe->frame) &&
			fbe->frame.seqNumber >= gbn_inst->expt_sq_nm) {
			push_frame_data_rcv_buffer(gbn_inst->is_srvr, sockfd, fbe);
			printf("GBN recvr pushing data frame into buffer\n");
			printf("seqNumber: %d, type: %d\n", rcv_frame.seqNumber, rcv_frame.type);
		}
		else {
			gbn_inst->snd_pkt.sockfd = gbn_inst->sockfd;
			gbn_inst->snd_pkt.msgType = ACK_M;

			gbn_inst->snd_frame.pkt = gbn_inst->snd_pkt;
			gbn_inst->snd_frame.seqNumber = gbn_inst->expt_sq_nm - 1;
			gbn_inst->snd_frame.type = ACK_F;

			char * checksum = compute_checksum(gbn_inst->snd_frame);
			int c;
			for (c = 0; c < CHAR_BIT; ++c)
				gbn_inst->snd_frame.checksum[c] = checksum[c];

			printf("Sending duplicate frame ACK for seqNumber %d, expected seq number is %d\n", gbn_inst->snd_frame.seqNumber, gbn_inst->expt_sq_nm);
			frame_acks_sent++;
			dup_frames_recvd++;
			udt_send(gbn_inst->is_srvr, gbn_inst->snd_frame);
		}
	}
	else {
		if (fbe->frame.seqNumber > 0 && !is_in_frame_ack_rcv_buffer(gbn_inst->is_srvr, sockfd, fbe->frame)) {
			push_frame_ack_rcv_buffer(gbn_inst->is_srvr, sockfd, fbe);
			printf("GBN sender pushing ack frame into buffer\n");
			printf("seqNumber: %d, type: %d\n", rcv_frame.seqNumber, rcv_frame.type);
		}
	}
	// destroy_fbe(fbe);
}

Frame * gbn_get_frame(GBN_instance * gbn_inst) {
	if (gbn_inst->is_sndr) {
		FrameBufferEntry * fbe = pop_frame_ack_rcv_buffer(gbn_inst->is_srvr, gbn_inst->is_sndr, gbn_inst->sockfd);
		if (fbe == NULL) {
			return NULL;
		}
		else {
			printf("GBN sender got an ack frame from the buffer\n");
			Frame * rcv_frame = &(fbe->frame);
			printf("seqNumber: %d, type: %d\n", rcv_frame->seqNumber, rcv_frame->type);
			return rcv_frame;
		}
	}
	else {
		FrameBufferEntry * fbe = pop_frame_data_rcv_buffer(gbn_inst->is_srvr, gbn_inst->is_sndr, gbn_inst->sockfd);
		if (fbe == NULL) {
			return NULL;
		}
		else {
			Frame * rcv_frame = &(fbe->frame);
			return rcv_frame;
		}
	}
}
// send frame = head of frame buffer
int gbn_rdt_send(GBN_instance * gbn_inst, int sockfd) {
	BufferEntry * pbe = pop_pkt_snd_buffer(gbn_inst->is_srvr, sockfd);

	if (pbe == NULL) 
		return 0;

	Packet pkt = pbe->pkt;

	FrameBufferEntry * fbe = malloc(sizeof(FrameBufferEntry));
	fbe->next = NULL;
	Frame frame;
	frame.pkt = pkt;
	frame.type = DATA_F;
	frame.seqNumber = gbn_inst->nxt_sq_nm;

	/*
	if (gbn_inst->is_sndr) {
		if (gbn_inst->is_srvr) {
			if (sockfd == client1_sockfd) {
				if (client1_data_snd_buffer.tail != NULL)
					frame.seqNumber = client1_data_snd_buffer.tail->frame.seqNumber + 1;
				else 
					frame.seqNumber = gbn_inst->base;
			}
			else {
				if (client2_data_snd_buffer.tail != NULL)
					frame.seqNumber = client2_data_snd_buffer.tail->frame.seqNumber + 1;
				else
					frame.seqNumber = gbn_inst->base;
			}
		}
		else {
			if (client_data_snd_buffer.tail != NULL)
				frame.seqNumber = client_data_snd_buffer.tail->frame.seqNumber + 1;
			else
				frame.seqNumber = gbn_inst->base;
		}
	}
	else {
		if (gbn_inst->is_srvr) {
			if (sockfd == client1_sockfd) {
				if (client1_ack_snd_buffer.tail != NULL)
					frame.seqNumber = client1_ack_snd_buffer.tail->frame.seqNumber + 1;
				else 
					frame.seqNumber = gbn_inst->base;
			}
			else {
				if (client2_ack_snd_buffer.tail != NULL)
					frame.seqNumber = client2_ack_snd_buffer.tail->frame.seqNumber + 1;
				else
					frame.seqNumber = gbn_inst->base;
			}
		}
		else {
			if (client_ack_snd_buffer.tail != NULL)
				frame.seqNumber = client2_ack_snd_buffer.tail->frame.seqNumber + 1;
			else
				frame.seqNumber = gbn_inst->base;
		}
	} 
	*/

	char * checksum = compute_checksum(frame);
	int c;
	for (c = 0; c < CHAR_BIT; ++c)
		frame.checksum[c] = checksum[c];
	fbe->frame = frame;

	if (fbe->frame.type == DATA_F) {
		printf("Pushing frame into frame send buffer\n");
		push_frame_data_snd_buffer(gbn_inst->is_srvr, sockfd, fbe);
	}
	// destroy_fbe(fbe);

	return 1;
}

void gbn_rdt_sendall(GBN_instance * gbn_inst) {
	FrameBufferEntry * fbe = NULL;

	if (gbn_inst->is_sndr) {
		if (gbn_inst->is_srvr) {
			if (gbn_inst->sockfd == client1_sockfd)
				fbe = client1_data_snd_buffer.head;
			else
				fbe = client2_data_snd_buffer.head;
		}
		else
			fbe = client_data_snd_buffer.head;

		if (fbe == NULL) 
			return;

		while (fbe != NULL && fbe->frame.seqNumber < gbn_inst->base) 
			fbe = fbe->next;

		while(fbe != NULL && fbe->frame.seqNumber < gbn_inst->nxt_sq_nm) {
			udt_send(gbn_inst->is_srvr, fbe->frame);
			fbe = fbe->next;
			data_frames_retransmits++;
		}
	}
	else {
		if (gbn_inst->is_srvr) {
			if (gbn_inst->sockfd == client1_sockfd)
				fbe = client1_ack_snd_buffer.head;
			else
				fbe = client2_ack_snd_buffer.head;
		}
		else
			fbe = client_ack_snd_buffer.head;

		if (fbe == NULL) 
			return;

		while (fbe != NULL && fbe->frame.seqNumber < gbn_inst->base) 
			fbe = fbe->next;

		while(fbe != NULL && fbe->frame.seqNumber < gbn_inst->nxt_sq_nm) {
			udt_send(gbn_inst->is_srvr, fbe->frame);
			fbe = fbe->next;
			data_frames_retransmits++;
		}
	}
}

void gbn_timeout(GBN_instance * gbn_inst) {
	// gbn_inst->is_timed 		= 0;
	gbn_inst->time_elpsd 	= 0.0;
	gbn_inst->start 		= clock();
	gbn_inst->diff 			= gbn_inst->start;
	printf("GBN sender timeout occured\n");
	gbn_rdt_sendall(gbn_inst);
}

void gbn_stop_time(GBN_instance * gbn_inst) {
	printf("GBN sender timer stopped\n");
	gbn_inst->is_timed 		= 0;
	gbn_inst->time_elpsd 	= 0.0;
	gbn_inst->start 		= clock();
	gbn_inst->diff 			= gbn_inst->start;
}
	
void gbn_start_time(GBN_instance * gbn_inst) {
	if (!gbn_inst->is_timed) {
		if (gbn_inst->last_ack_rcvd > 0)
			printf("GBN sender timer started\n");
		gbn_inst->is_timed 		= 1;
		gbn_inst->time_elpsd 	= 0.0;
		gbn_inst->start 		= clock();
		gbn_inst->diff 			= gbn_inst->start;
	}
}

int gbn_update_timer(GBN_instance * gbn_inst) {
	if (gbn_inst->is_timed) {
		gbn_inst->diff = clock() - gbn_inst->start;
		gbn_inst->time_elpsd = ((double)(gbn_inst->diff)/(double)CLOCKS_PER_SEC);

		// printf("time elapsed %f\n", gbn_inst->time_elpsd);
		if (gbn_inst->time_elpsd > gbn_inst->time_max) 
			return 1;
	}
	return 0; // 1 if timeout
}
	
void update_gbn(GBN_instance * gbn_inst) {
	Frame * rcv_frame = NULL;	
	if (gbn_inst->is_sndr) { //sender fsm
		int timeout = gbn_update_timer(gbn_inst); // already checks if timer is on or off, has timeout detection
		if (gbn_rdt_send(gbn_inst, gbn_inst->sockfd)) {
			if (gbn_inst->nxt_sq_nm < gbn_inst->base + gbn_inst->wndw_sz) {
				FrameBufferEntry * fbe = get_from_data_frame_snd_buffer(gbn_inst->is_srvr, gbn_inst->sockfd, gbn_inst->nxt_sq_nm);
				if (fbe != NULL) {
					udt_send(gbn_inst->is_srvr, fbe->frame);
				}
				if (gbn_inst->base == gbn_inst->nxt_sq_nm)
					gbn_start_time(gbn_inst);

				gbn_inst->nxt_sq_nm++;
				printf("NEW NEXT SEQUENCE NUMBER %d\n", gbn_inst->nxt_sq_nm);
			}
		}

		rcv_frame = gbn_get_frame(gbn_inst);
		if (rcv_frame != NULL && !corrupt_frame(*rcv_frame)) {
			int old_base = gbn_inst->base;

			gbn_inst->last_ack_rcvd = rcv_frame->seqNumber;
			gbn_inst->base = rcv_frame->seqNumber + 1;
			printf("Sender got an ack %d %d %d!\n", gbn_inst->base, gbn_inst->nxt_sq_nm, rcv_frame->seqNumber);
			frame_acks_recvd++;
			if (gbn_inst->base == gbn_inst->nxt_sq_nm) {
				gbn_stop_time(gbn_inst);

				int b;
				for (b = old_base; b < gbn_inst->base; ++b) {
					printf("Removing frame with seqNumber %d from frame send buffer\n", b);
					remove_from_frame_data_snd_buffer(gbn_inst->is_srvr, gbn_inst->sockfd, gbn_inst->nxt_sq_nm);
				}
			}
			else
				gbn_start_time(gbn_inst);
		}

		if (timeout) 
			gbn_timeout(gbn_inst);

	}
	else { //recvr
		rcv_frame = gbn_get_frame(gbn_inst);
		if (rcv_frame != NULL) {
			gbn_inst->snd_pkt.sockfd = gbn_inst->sockfd;
			gbn_inst->snd_pkt.msgType = ACK_M;

			gbn_inst->snd_frame.pkt = gbn_inst->snd_pkt;
			gbn_inst->snd_frame.seqNumber = gbn_inst->expt_sq_nm;
			gbn_inst->snd_frame.type = ACK_F;

			char * checksum = compute_checksum(gbn_inst->snd_frame);
			int c;
			for (c = 0; c < CHAR_BIT; ++c)
				gbn_inst->snd_frame.checksum[c] = checksum[c];

			if (!corrupt_frame(*rcv_frame) && (rcv_frame->seqNumber == gbn_inst->expt_sq_nm)) {

				BufferEntry * pbe = malloc(sizeof (BufferEntry));
				Packet data = rcv_frame->pkt;
				pbe->pkt = data;
				pbe->next = NULL;	
				
				gbn_inst->expt_sq_nm++;
				printf("NEW EXPECTED SEQ NUM %d\n", gbn_inst->expt_sq_nm);

				printf("Delivering data packet to application layer\n");
				deliver_data(gbn_inst->is_srvr, gbn_inst->sockfd, pbe);

				printf("Sending ACK for seqNumber %d, expected seq number is %d\n", gbn_inst->snd_frame.seqNumber, gbn_inst->expt_sq_nm);
				frame_acks_sent++;
				udt_send(gbn_inst->is_srvr, gbn_inst->snd_frame);				
			}
			else {
				gbn_inst->snd_frame.seqNumber = gbn_inst->expt_sq_nm - 1;

				char * checksum = compute_checksum(gbn_inst->snd_frame);
				int c;
				for (c = 0; c < CHAR_BIT; ++c)
					gbn_inst->snd_frame.checksum[c] = checksum[c];

				printf("Sending corrupt or out of order frame ACK for seqNumber %d\n", gbn_inst->snd_frame.seqNumber);
				frame_acks_sent++;
				udt_send(gbn_inst->is_srvr, gbn_inst->snd_frame);
			}
		}
	}

	// if (rcv_frame != NULL)
	// 	free(rcv_frame);
	// memset( rcv_frame, 0, sizeof(Frame) );
}	

#endif
