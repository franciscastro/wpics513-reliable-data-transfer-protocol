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

/*
	GBN sender and rcvr linked lists structs (for server side)
	Used to keep track and update gbn_instances for each socket / client
	Linked-list implementation allows for dynamic sizing
*/
typedef struct GBN_instance_sndr_list {
	GBN_instance * head;
	GBN_instance * tail;
	int count;
} GBN_instance_sndr_list;

typedef struct GBN_instance_rcvr_list {
	struct GBN_instance * head;
	struct GBN_instance * tail;
	int count;
} GBN_instance_rcvr_list;

void insert_gbn_sndr(GBN_instance_sndr_list list, GBN_instance * gbn_inst) {
	if (list.head == NULL) {
		list.head = gbn_inst;
		list.tail = gbn_inst;
	}
	else {
		gbn_inst->prev = list.tail;
		list.tail->next = gbn_inst;
		list.tail = gbn_inst;
	}
}

int remove_gbn_sndr(GBN_instance_sndr_list list, GBN_instance * gbn_inst) {
	if (list.head == NULL)
		return false;
	GBN_instance * curr = list.head;
	while (curr != gbn_inst && curr != NULL) {
		curr = curr->next;
	}
	if (curr == NULL)
		return false;
	else {
		curr->prev->next = curr->next;
		curr->next->prev = curr->prev;
		Destroy_GBN_instance(curr);
	}
	return true;
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

void gbn_insert_frame(GBN_instance * gbn_inst, Frame data) {
	FrameBufferEntry * fb_entry = malloc(sizeof( FrameBufferEntry));
	fb_entry->frame = data;
	fb_entry->next = NULL;

	if (gbn_inst->frame_head == NULL) {
		gbn_inst->frame_head = fb_entry;
		gbn_inst->frame_tail = fb_entry;
		fb_entry->count = 1;
	}
	else {
		gbn_inst->frame_tail->next = fb_entry;
		fb_entry->count = gbn_inst->frame_tail->count + 1;
		gbn_inst->frame_tail = fb_entry;
	}

}

FrameBufferEntry* gbn_pop_frame(GBN_instance * gbn_inst) {
	if (gbn_inst->frame_head == NULL) 
		return;
	FrameBufferEntry * ret = gbn_inst->frame_head;
	gbn_inst->frame_head = ret->next;

	ret->next = NULL;
	return ret;
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

	push_frame_rcv_buffer(gbn_inst->is_srvr, sockfd, fbe);
}

Frame * gbn_get_frame(GBN_instance * gbn_inst) {
	FrameBufferEntry * fbe = pop_frame_rcv_buffer(gbn_inst->is_srvr, gbn_inst->is_sndr, gbn_inst->sockfd);
	if (fbe == NULL) {
		return NULL;
	}
	else {
		Frame * rcv_frame = &(fbe->frame);
		if (rcv_frame->type != ACK_F) {
			// printf("Got a frame from the framebuffer! %d\n", (rcv_frame == NULL));
			// printf("Frame has seq number %d\n", rcv_frame->seqNumber);
		}
		return rcv_frame;
	}

}
// send frame = head of frame buffer
int gbn_rdt_send(GBN_instance * gbn_inst, int sockfd) {
	BufferEntry * pbe = pop_pkt_snd_buffer(gbn_inst->is_srvr, sockfd);

	if (pbe == NULL) 
		return 0;

	gbn_inst->nxt_sq_nm++;

	Packet pkt = pbe->pkt;

	FrameBufferEntry * fbe = malloc(sizeof(FrameBufferEntry));
	fbe->next = NULL;
	Frame frame;
	frame.pkt = pkt;
	frame.type = DATA_F;
	// frame.seqNumber = gbn_inst->nxt_sq_nm;

	if (gbn_inst->is_srvr) {
		if (sockfd == client1_sockfd) {
			if (client1_snd_buffer.tail != NULL)
				frame.seqNumber = client1_snd_buffer.tail->frame.seqNumber + 1;
			else 
				frame.seqNumber = gbn_inst->base;
		}
		else {
			if (client2_snd_buffer.tail != NULL)
				frame.seqNumber = client2_snd_buffer.tail->frame.seqNumber + 1;
			else
				frame.seqNumber = gbn_inst->base;
		}
	}
	else {
		if (client_snd_buffer.tail != NULL)
			frame.seqNumber = client_snd_buffer.tail->frame.seqNumber + 1;
		else
			frame.seqNumber = gbn_inst->base;
	}


	char * checksum = compute_checksum(frame);
	int c;
	for (c = 0; c < CHAR_BIT; ++c)
		frame.checksum[c] = checksum[c];
	// strcpy(frame.checksum, compute_checksum(frame));
	printf("gbn_rdt_send frame with checksum %s from %s and seqNumber %d and is sender nxt_sq_nm %d\n", frame.checksum, checksum, frame.seqNumber, gbn_inst->nxt_sq_nm);
	fbe->frame = frame;
	push_frame_snd_buffer(gbn_inst->is_srvr, sockfd, fbe);

	return 1;
}

void gbn_rdt_sendall(GBN_instance * gbn_inst) {
	FrameBufferEntry * fbe = NULL;

	if (gbn_inst->is_srvr) {
		if (gbn_inst->sockfd == client1_sockfd)
			fbe = client1_snd_buffer.head;
		else
			fbe = client2_snd_buffer.head;
	}
	else
		fbe = client_snd_buffer.head;

	if (fbe == NULL)
		return;

	while (fbe != NULL && fbe->frame.seqNumber < gbn_inst->base) 
		fbe = fbe->next;

	printf("gbn_inst->base %d and gbn_inst->nxt_sq_nm %d\n", gbn_inst->base, gbn_inst->nxt_sq_nm);
	while(fbe != NULL && fbe->frame.seqNumber < gbn_inst->nxt_sq_nm) {
		printf("GBN sender retransmitting seqNumber %d\n", fbe->frame.seqNumber);
		udt_send(gbn_inst->is_srvr, fbe->frame);

		if (fbe->next == NULL) {
			printf("No more frames to be retransmitted\n");
		}
		fbe = fbe->next;
	}
}

void gbn_timeout(GBN_instance * gbn_inst) {
	// printf("GBN sender timeout occured\n");
	gbn_rdt_sendall(gbn_inst);
}

void gbn_stop_time(GBN_instance * gbn_inst) {
	// printf("GBN sender timer stopped\n");
	gbn_inst->is_timed = 0;
	gbn_inst->time_elpsd = 0.0;
}

void gbn_start_time(GBN_instance * gbn_inst) {
	// printf("GBN sender timer started\n");
	if (!gbn_inst->is_timed) {
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
			printf("Sending in sndr FSM\n");
			if (gbn_inst->nxt_sq_nm < gbn_inst->base + gbn_inst->wndw_sz) {
				FrameBufferEntry * fbe = get_from_frame_snd_buffer(gbn_inst->is_srvr, gbn_inst->sockfd, gbn_inst->nxt_sq_nm);
				if (fbe != NULL) {
					printf("Sending frame with seqNumber %d\n", fbe->frame.seqNumber);
					udt_send(gbn_inst->is_srvr, fbe->frame);
				}
				printf("Comparing time in sndr fsm\n");
				if (gbn_inst->base == gbn_inst->nxt_sq_nm)
					gbn_start_time(gbn_inst);
				printf("Incrementing next seq number in sndr fsm\n");
			}
		}
		
		if (timeout) 
			gbn_timeout(gbn_inst);

		rcv_frame = gbn_get_frame(gbn_inst);
		if (rcv_frame != NULL && !corrupt_frame(*rcv_frame)) {
			// printf("Receiving in sndr FSM\n");
			if (gbn_inst->is_srvr == 1 && rcv_frame->seqNumber > 0)
				printf("Got data frame\n");

			int old_base = gbn_inst->base;
			gbn_inst->base = rcv_frame->seqNumber + 1;
			if (gbn_inst->base == gbn_inst->nxt_sq_nm) {
				gbn_stop_time(gbn_inst);

				//remove old frames from the frame send buffer
				int b;
				for (b = old_base; b < gbn_inst->base; ++b) {
					printf("Removing frame with seqNumber %d from frame send buffer\n", b);
					remove_from_frame_snd_buffer(gbn_inst->is_srvr, gbn_inst->sockfd, gbn_inst->nxt_sq_nm);
				}
			}
			else
				gbn_start_time(gbn_inst);
		}
		else {	
			// do nothing (according to FSM)
		}
	}
	else { //recver
		rcv_frame = gbn_get_frame(gbn_inst);

		// if (rcv_frame != NULL) 
		// 	printf("rcvd frame with seq number %d, expecting seq number %d\n", rcv_frame->seqNumber, gbn_inst->expt_sq_nm);	

		if (rcv_frame != NULL && !corrupt_frame(*rcv_frame) && (rcv_frame->seqNumber == gbn_inst->expt_sq_nm)) {

			BufferEntry * pbe = malloc(sizeof (BufferEntry));
			Packet data = rcv_frame->pkt;
			pbe->pkt = data;
			pbe->next = NULL;

			gbn_inst->snd_pkt.sockfd = gbn_inst->sockfd;
			gbn_inst->snd_pkt.msgType = ACK_M;

			gbn_inst->snd_frame.pkt = gbn_inst->snd_pkt;
			gbn_inst->snd_frame.seqNumber = gbn_inst->expt_sq_nm;
			gbn_inst->snd_frame.type = ACK_F;

			char * checksum = compute_checksum(gbn_inst->snd_frame);
			int c;
			for (c = 0; c < CHAR_BIT; ++c)
				gbn_inst->snd_frame.checksum[c] = checksum[c];
			// strcpy(gbn_inst->snd_frame.checksum, compute_checksum(gbn_inst->snd_frame));

			printf("Sending ACK for seqNumber %d, expected seq number is %d\n", gbn_inst->snd_frame.seqNumber, gbn_inst->expt_sq_nm);
			udt_send(gbn_inst->is_srvr, gbn_inst->snd_frame);
			
			gbn_inst->expt_sq_nm++;

			printf("Delivering data packet to application layer\n");
			deliver_data(gbn_inst->is_srvr, gbn_inst->sockfd, pbe);
		}
		else {
			// free(rcv_frame);
			// client sending only test
			if (gbn_inst->is_srvr) {
				// printf("Sending ACK frame with seqNumber %d\n", gbn_inst->snd_frame.seqNumber);
				udt_send(gbn_inst->is_srvr, gbn_inst->snd_frame);
			}
		}
	}
}	

//========================TEST FUNCTIONS=============================

/*
	sending without gbn_update
*/
void test_gbn_rdt_send(GBN_instance * gbn_inst, Packet pkt) {
	printf("test_gbn_rdt_send %d %d %d\n", gbn_inst->nxt_sq_nm, gbn_inst->base, gbn_inst->wndw_sz);
	printf("is valid? %d\n", gbn_inst->nxt_sq_nm < gbn_inst->base + gbn_inst->wndw_sz);
	if (gbn_inst->nxt_sq_nm < gbn_inst->base + gbn_inst->wndw_sz) {
		Frame to_send = make_frame(gbn_inst->is_srvr, pkt, DATA_F, gbn_inst->nxt_sq_nm, 0);
		printf("sending frame \n");
		printf("pkt msgType %d\n", to_send.pkt.msgType);
		printf("seqNumber %d\n", to_send.seqNumber);

		udt_send(gbn_inst->is_srvr, to_send);

		if (gbn_inst->base == gbn_inst->nxt_sq_nm) {
			gbn_start_time(gbn_inst);
			gbn_inst->nxt_sq_nm += 1;
		}
		else {
			refuse_data(gbn_inst->is_srvr, to_send);
		}
	}
}
/*
	end sending without gbn_update
*/

#endif
