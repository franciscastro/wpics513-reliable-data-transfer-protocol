/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Selective repet ARQ file.
*/

#ifndef SRPT_H_
#define SRPT_H_

#include "config.h";
#include "datalink.h"
#include <time.h>

int srpt_wndw_sz = 10;
double srpt_max_time = 0.5;

/*
	srpt struct used to store information needed to perform ARQ
	Required because server side needs multiple different srpt
		values (one for each socket/client)
*/

typedef struct srptThreadData {
	pthread_t thread_ID;		// This thread's pointer
} srptThreadData;

typedef struct SRPT_instance {

    double time_elpsd;
    double time_max;

    int sockfd;
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
    struct SRPT_instance * next;
    struct SRPT_instance * prev;

    Packet snd_pkt;
    Frame snd_frame;
} SRPT_instance;

void Destroy_SRPT_instance(SRPT_instance * srpt_inst) {
	// fix this later -> must null and free all members of the linked list
	srpt_inst->pkt_head = NULL;
	srpt_inst->pkt_tail = NULL;

	srpt_inst->frame_head = NULL;
	srpt_inst->frame_tail = NULL;

	srpt_inst->next = NULL;
	srpt_inst->prev = NULL;

	free(srpt_inst);
}

/*
	srpt sender and rcvr linked lists structs (for server side)
	Used to keep track and update SRPT_instances for each socket / client
	Linked-list implementation allows for dynamic sizing
*/
typedef struct SRPT_instance_sndr_list {
	SRPT_instance * head;
	SRPT_instance * tail;
	int count;
} SRPT_instance_sndr_list;

typedef struct SRPT_instance_rcvr_list {
	struct SRPT_instance * head;
	struct SRPT_instance * tail;
	int count;
} SRPT_instance_rcvr_list;

void insert_srpt_sndr(SRPT_instance_sndr_list list, SRPT_instance * srpt_inst) {
	if (list.head == NULL) {
		list.head = srpt_inst;
		list.tail = srpt_inst;
	}
	else {
		srpt_inst->prev = list.tail;
		list.tail->next = srpt_inst;
		list.tail = srpt_inst;
	}
}

int remove_srpt_sndr(SRPT_instance_sndr_list list, SRPT_instance * srpt_inst) {
	if (list.head == NULL)
		return false;
	SRPT_instance * curr = list.head;
	while (curr != srpt_inst && curr != NULL) {
		curr = curr->next;
	}
	if (curr == NULL)
		return false;
	else {
		curr->prev->next = curr->next;
		curr->next->prev = curr->prev;
		Destroy_SRPT_instance(curr);
	}
	return true;
}


void srpt_init(int sockfd, int is_sndr, int is_srvr, int wndw_sz, double time_max, SRPT_instance * srpt_inst) {
	srand((unsigned) time(&t));

	srpt_inst->sockfd		= sockfd; // which socket I'm listening to
	srpt_inst->is_sndr 		= is_sndr;
	srpt_inst->is_srvr 		= is_srvr;
	srpt_inst->time_max 		= time_max;
	srpt_inst->wndw_sz 		= wndw_sz;
	srpt_inst->time_elpsd 	= 0.0;
	srpt_inst->is_timed		= 0;
	srpt_inst->frame_bfr_sz 	= 0;
	srpt_inst->pkt_bfr_sz	= 0;
	srpt_inst->base 			= 0;
	srpt_inst->nxt_sq_nm 	= 0;
	srpt_inst->frame_head 	= NULL;
	srpt_inst->frame_tail 	= NULL;
	srpt_inst->pkt_head 		= NULL;
	srpt_inst->pkt_tail 		= NULL;

	if (is_sndr) {
		srpt_inst->base = 1;
		srpt_inst->nxt_sq_nm = 1;	
	}
	else {
		srpt_inst->expt_sq_nm = 1;
		srpt_inst->snd_pkt.sockfd = sockfd;
		srpt_inst->snd_frame.pkt = srpt_inst->snd_pkt;
		srpt_inst->snd_frame.type = ACK_F;
		srpt_inst->snd_frame.seqNumber = 0;

		char * checksum = compute_checksum(srpt_inst->snd_frame);
		int c;
		for (c = 0; c < CHAR_BIT; ++c)
			srpt_inst->snd_frame.checksum[c] = checksum[c];
		// strcpy(srpt_inst->snd_frame.checksum, compute_checksum(srpt_inst->snd_frame));
	}
}

void srpt_insert_frame(SRPT_instance * srpt_inst, Frame data) {
	FrameBufferEntry * fb_entry = malloc(sizeof( FrameBufferEntry));
	fb_entry->frame = data;
	fb_entry->next = NULL;

	if (srpt_inst->frame_head == NULL) {
		srpt_inst->frame_head = fb_entry;
		srpt_inst->frame_tail = fb_entry;
		fb_entry->count = 1;
	}
	else {
		srpt_inst->frame_tail->next = fb_entry;
		fb_entry->count = srpt_inst->frame_tail->count + 1;
		srpt_inst->frame_tail = fb_entry;
	}

}

FrameBufferEntry* srpt_pop_frame(SRPT_instance * srpt_inst) {
	if (srpt_inst->frame_head == NULL) 
		return;
	FrameBufferEntry * ret = srpt_inst->frame_head;
	srpt_inst->frame_head = ret->next;

	ret->next = NULL;
	return ret;
}


/*
	Called by physical layer to push a FrameBufferEntry (containing the received frame) into its
	corresponding FrameBuffer (determined by srpt type [server/client] and socket which the frame 
	came from)

	Have to hadle ACK frames differently (don't store them into packet rcvr)
*/
void srpt_rdt_rcv(SRPT_instance * srpt_inst, Frame rcv_frame, int sockfd) {
	FrameBufferEntry * fbe = malloc(sizeof(FrameBufferEntry));
	fbe->frame = rcv_frame;
	fbe->next = NULL;

	// if (srpt_inst->is_sndr && rcv_frame.seqNumber > 0) {
	// 	printf("srpt sender pushing ack frame into buffer\n");
	// 	printf("seqNumber: %d, type: %d\n", rcv_frame.seqNumber, rcv_frame.type);
	// }
	push_frame_rcv_buffer(srpt_inst->is_srvr, sockfd, fbe);
}

Frame * srpt_get_frame(SRPT_instance * srpt_inst) {
	FrameBufferEntry * fbe = pop_frame_rcv_buffer(srpt_inst->is_srvr, srpt_inst->is_sndr, srpt_inst->sockfd);
	if (fbe == NULL) {
		return NULL;
	}
	else {
		Frame * rcv_frame = &(fbe->frame);
		//if (rcv_frame->type == ACK_F && rcv_frame->seqNumber > 0) {
		// if (srpt_inst->is_sndr && rcv_frame->seqNumber > 0) {
		// 	printf("Got ack frame from the framebuffer! %d\n", (rcv_frame == NULL));
		// 	printf("Frame has seq number %d\n", rcv_frame->seqNumber);
		// 	printf("Frame has type %d\n", rcv_frame->type);
		// }
		return rcv_frame;
	}

}
// send frame = head of frame buffer
int srpt_rdt_send(SRPT_instance * srpt_inst, int sockfd) {
	BufferEntry * pbe = pop_pkt_snd_buffer(srpt_inst->is_srvr, sockfd);

	if (pbe == NULL) 
		return 0;

	srpt_inst->nxt_sq_nm++;

	Packet pkt = pbe->pkt;

	FrameBufferEntry * fbe = malloc(sizeof(FrameBufferEntry));
	fbe->next = NULL;
	Frame frame;
	frame.pkt = pkt;
	frame.type = DATA_F;
	// frame.seqNumber = srpt_inst->nxt_sq_nm;

	if (srpt_inst->is_srvr) {
		if (sockfd == client1_sockfd) {
			if (client1_snd_buffer.tail != NULL)
				frame.seqNumber = client1_snd_buffer.tail->frame.seqNumber + 1;
			else 
				frame.seqNumber = srpt_inst->base;
		}
		else {
			if (client2_snd_buffer.tail != NULL)
				frame.seqNumber = client2_snd_buffer.tail->frame.seqNumber + 1;
			else
				frame.seqNumber = srpt_inst->base;
		}
	}
	else {
		if (client_snd_buffer.tail != NULL)
			frame.seqNumber = client_snd_buffer.tail->frame.seqNumber + 1;
		else
			frame.seqNumber = srpt_inst->base;
	}


	char * checksum = compute_checksum(frame);
	int c;
	for (c = 0; c < CHAR_BIT; ++c)
		frame.checksum[c] = checksum[c];
	// strcpy(frame.checksum, compute_checksum(frame));
	fbe->frame = frame;
	push_frame_snd_buffer(srpt_inst->is_srvr, sockfd, fbe);

	return 1;
}

void srpt_rdt_sendall(SRPT_instance * srpt_inst) {
	FrameBufferEntry * fbe = NULL;

	if (srpt_inst->is_srvr) {
		if (srpt_inst->sockfd == client1_sockfd)
			fbe = client1_snd_buffer.head;
		else
			fbe = client2_snd_buffer.head;
	}
	else
		fbe = client_snd_buffer.head;

	if (fbe == NULL)
		return;

	while (fbe != NULL && fbe->frame.seqNumber < srpt_inst->base) 
		fbe = fbe->next;

	// printf("srpt_inst->base %d and srpt_inst->nxt_sq_nm %d\n", srpt_inst->base, srpt_inst->nxt_sq_nm);
	while(fbe != NULL && fbe->frame.seqNumber < srpt_inst->nxt_sq_nm) {
		printf("srpt sender retransmitting seqNumber %d\n", fbe->frame.seqNumber);
		udt_send(srpt_inst->is_srvr, fbe->frame);

		// if (fbe->next == NULL) {
		// 	printf("No more frames to be retransmitted\n");
		// }
		fbe = fbe->next;
	}
}

void srpt_timeout(SRPT_instance * srpt_inst) {
	// printf("srpt sender timeout occured\n");
	srpt_rdt_sendall(srpt_inst);
}

void srpt_stop_time(SRPT_instance * srpt_inst) {
	// printf("srpt sender timer stopped\n");
	srpt_inst->is_timed = 0;
	srpt_inst->time_elpsd = 0.0;
}

void srpt_start_time(SRPT_instance * srpt_inst) {
	// printf("srpt sender timer started\n");
	if (!srpt_inst->is_timed) {
		srpt_inst->is_timed 		= 1;
		srpt_inst->time_elpsd 	= 0.0;
		srpt_inst->start 		= clock();
		srpt_inst->diff 			= srpt_inst->start;
	}
}

int srpt_update_timer(SRPT_instance * srpt_inst) {
	if (srpt_inst->is_timed) {
		srpt_inst->diff = clock() - srpt_inst->start;
		srpt_inst->time_elpsd = ((double)(srpt_inst->diff)/(double)CLOCKS_PER_SEC);

		if (srpt_inst->time_elpsd > srpt_inst->time_max) 
			return 1;
	}
	return 0; // 1 if timeout
}
	
void update_srpt(SRPT_instance * srpt_inst) {
	Frame * rcv_frame = NULL;	
	if (srpt_inst->is_sndr) { //sender fsm
		int timeout = srpt_update_timer(srpt_inst); // already checks if timer is on or off, has timeout detection
		if (srpt_rdt_send(srpt_inst, srpt_inst->sockfd)) {
			if (srpt_inst->nxt_sq_nm < srpt_inst->base + srpt_inst->wndw_sz) {
				FrameBufferEntry * fbe = get_from_frame_snd_buffer(srpt_inst->is_srvr, srpt_inst->sockfd, srpt_inst->nxt_sq_nm);
				if (fbe != NULL) {
					udt_send(srpt_inst->is_srvr, fbe->frame);
				}
				if (srpt_inst->base == srpt_inst->nxt_sq_nm)
					srpt_start_time(srpt_inst);
			}
		}
		
		if (timeout) 
			srpt_timeout(srpt_inst);

		rcv_frame = srpt_get_frame(srpt_inst);
		if (rcv_frame != NULL && !corrupt_frame(*rcv_frame)) {
			// printf("Receiving in sndr FSM\n");
			// if (srpt_inst->is_srvr == 0 && rcv_frame->seqNumber > 0)
			// 	printf("Got ACK frame %d, base is %d, and nxt_sq_nm %d, progress? %d\n", rcv_frame->seqNumber, srpt_inst->base, srpt_inst->nxt_sq_nm, (srpt_inst->base + 1)== srpt_inst->nxt_sq_nm);

			int old_base = srpt_inst->base;
			srpt_inst->base = rcv_frame->seqNumber + 1;
			if (srpt_inst->base == srpt_inst->nxt_sq_nm) {
				srpt_stop_time(srpt_inst);

				//remove old frames from the frame send buffer
				int b;

				// if (rcv_frame->seqNumber > 0)
				// 	printf("OLD BASE: %d, NEW BASE: %d\n", old_base, srpt_inst->base);
				for (b = old_base; b < srpt_inst->base; ++b) {
					// printf("Removing frame with seqNumber %d from frame send buffer\n", b);
					remove_from_frame_snd_buffer(srpt_inst->is_srvr, srpt_inst->sockfd, srpt_inst->nxt_sq_nm);
				}
			}
			else
				srpt_start_time(srpt_inst);
		}
		else {	
			rcv_frame = NULL;
			// do nothing (according to FSM)
		}
	}
	else { //recver
		rcv_frame = srpt_get_frame(srpt_inst);

		// if (rcv_frame != NULL) 
		// 	printf("rcvd frame with seq number %d, expecting seq number %d\n", rcv_frame->seqNumber, srpt_inst->expt_sq_nm);	

		if (rcv_frame != NULL && !corrupt_frame(*rcv_frame) && (rcv_frame->seqNumber == srpt_inst->expt_sq_nm)) {

			BufferEntry * pbe = malloc(sizeof (BufferEntry));
			Packet data = rcv_frame->pkt;
			pbe->pkt = data;
			pbe->next = NULL;

			srpt_inst->snd_pkt.sockfd = srpt_inst->sockfd;
			srpt_inst->snd_pkt.msgType = ACK_M;

			srpt_inst->snd_frame.pkt = srpt_inst->snd_pkt;
			srpt_inst->snd_frame.seqNumber = srpt_inst->expt_sq_nm;
			srpt_inst->snd_frame.type = ACK_F;

			char * checksum = compute_checksum(srpt_inst->snd_frame);
			int c;
			for (c = 0; c < CHAR_BIT; ++c)
				srpt_inst->snd_frame.checksum[c] = checksum[c];
			// strcpy(srpt_inst->snd_frame.checksum, compute_checksum(srpt_inst->snd_frame));

			printf("Sending ACK for seqNumber %d, expected seq number is %d\n", srpt_inst->snd_frame.seqNumber, srpt_inst->expt_sq_nm);
			udt_send(srpt_inst->is_srvr, srpt_inst->snd_frame);
			
			srpt_inst->expt_sq_nm++;

			printf("Delivering data packet to application layer\n");
			deliver_data(srpt_inst->is_srvr, srpt_inst->sockfd, pbe);
		}
		else {
			// free(rcv_frame);
			// client sending only test
			if (srpt_inst->is_srvr) {
				// printf("Sending ACK frame with seqNumber %d\n", srpt_inst->snd_frame.seqNumber);
				udt_send(srpt_inst->is_srvr, srpt_inst->snd_frame);
			}
		}
	}
}	

//========================TEST FUNCTIONS=============================

/*
	sending without srpt_update
*/
void test_srpt_rdt_send(SRPT_instance * srpt_inst, Packet pkt) {
	printf("test_srpt_rdt_send %d %d %d\n", srpt_inst->nxt_sq_nm, srpt_inst->base, srpt_inst->wndw_sz);
	printf("is valid? %d\n", srpt_inst->nxt_sq_nm < srpt_inst->base + srpt_inst->wndw_sz);
	if (srpt_inst->nxt_sq_nm < srpt_inst->base + srpt_inst->wndw_sz) {
		Frame to_send = make_frame(srpt_inst->is_srvr, pkt, DATA_F, srpt_inst->nxt_sq_nm, 0);
		printf("sending frame \n");
		printf("pkt msgType %d\n", to_send.pkt.msgType);
		printf("seqNumber %d\n", to_send.seqNumber);

		udt_send(srpt_inst->is_srvr, to_send);

		if (srpt_inst->base == srpt_inst->nxt_sq_nm) {
			srpt_start_time(srpt_inst);
			srpt_inst->nxt_sq_nm += 1;
		}
		else {
			refuse_data(srpt_inst->is_srvr, to_send);
		}
	}
}
/*
	end sending without srpt_update
*/

#endif
