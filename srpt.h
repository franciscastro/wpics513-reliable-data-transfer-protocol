/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Go-Back-N ARQ file.
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
typedef struct SRPT_instance {

    double time_elpsd;
    double time_max;

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
} SRPT_instance;

void Destroy_SRPT_instance(SRPT_instance * srpt_inst) {
	// free(srpt_inst->is_srvr);
	// free(srpt_inst->is_sndr);
	// free(srpt_inst->is_timed);

	// free(srpt_inst->time_elpsd);
	// free(srpt_inst->time_max);
	// free(srpt_inst->wndw_sz);
	// free(srpt_inst->base);
	// free(srpt_inst->nxt_sq_nm);
	// free(srpt_inst->expt_sq_nm);
	// free(srpt_inst->pkt_bfr_sz);
	// free(srpt_inst->frame_bfr_sz);

	// free(srpt_inst->start);
	// free(srpt_inst->diff);

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

void start_srpt_rcvr_thread() {
	SRPT_instance srpt_inst;
}

void start_srpt_sndr_thread() {
	SRPT_instance srpt_inst;
}

void srpt_init(int is_sndr, int is_srvr, int wndw_sz, double time_max, SRPT_instance srpt_inst) {
	srpt_inst.is_sndr 		= is_sndr;
	srpt_inst.is_srvr 		= is_srvr;
	srpt_inst.time_max 		= time_max;
	srpt_inst.wndw_sz 		= wndw_sz;
	srpt_inst.time_elpsd 	= 0.0;
	srpt_inst.is_timed		= 0;
	srpt_inst.frame_bfr_sz 	= 0;
	srpt_inst.pkt_bfr_sz		= 0;
	srpt_inst.base 			= 0;
	srpt_inst.nxt_sq_nm 		= 0;
	srpt_inst.frame_head 	= NULL;
	srpt_inst.frame_tail 	= NULL;
	srpt_inst.pkt_head 		= NULL;
	srpt_inst.pkt_tail 		= NULL;

	if (is_sndr) {
		srpt_inst.expt_sq_nm = 1;
		//Frame snd_frame = //what???
	}
}

void srpt_insert_frame(SRPT_instance srpt_inst, Frame data) {
	FrameBufferEntry * fb_entry = malloc(sizeof( FrameBufferEntry));
	fb_entry->frame = data;

	if (srpt_inst.frame_head == NULL) {
		srpt_inst.frame_head = fb_entry;
		srpt_inst.frame_tail = fb_entry;
		fb_entry->count = 1;
	}
	else {
		srpt_inst.frame_tail->next = fb_entry;
		fb_entry->count = srpt_inst.frame_tail->count + 1;
		srpt_inst.frame_tail = fb_entry;
	}

}

FrameBufferEntry* srpt_pop_frame(SRPT_instance srpt_inst) {
	if (srpt_inst.frame_head == NULL) 
		return;
	FrameBufferEntry * ret = srpt_inst.frame_head;
	srpt_inst.frame_head = ret->next;

	ret->next = NULL;
	return ret;
}

void update_srpt(SRPT_instance srpt_inst) {
	srpt_update_timer(srpt_inst);
}

void srpt_recv(SRPT_instance srpt_inst, Frame rcv_frame) {
	if (srpt_inst.is_sndr) {
		if (!corrupt_frame(rcv_frame)) {
			srpt_inst.base = rcv_frame.nextAckExpected + 1;
			if (srpt_inst.base == srpt_inst.nxt_sq_nm)
				srpt_stop_time(srpt_inst);
			else
				srpt_start_time(srpt_inst);
		}
	}
	else {
		// if(!corrupt_frame(rcv_frame) && rcv_frame)
	}
}

// send frame = head of frame buffer
void srpt_rdt_send(SRPT_instance srpt_inst, Packet pkt) {
	if (srpt_inst.nxt_sq_nm < srpt_inst.base + srpt_inst.wndw_sz) {
		rdt_send(srpt_inst.is_srvr, pkt);
		if (srpt_inst.base == srpt_inst.nxt_sq_nm) {
			srpt_start_time(srpt_inst);
			srpt_inst.nxt_sq_nm += 1;
		}
	}
	else {
		// refuse_data(pkt);
	}
}

void srpt_rdt_rcv(SRPT_instance srpt_inst, Frame frame) {
	// srpt_inst.frame_buffer[srpt_inst.nxt_sq_nm] = 
}

void srpt_timeout(SRPT_instance srpt_inst) {
	srpt_start_time(srpt_inst);
	int i;
	for (i = 0; i < srpt_inst.pkt_bfr_sz; ++i) {
		// rdt_send(srpt_inst.is_srvr, srpt_inst.packet_buffer[i]);
	}
}

void srpt_stop_time(SRPT_instance srpt_inst) {
	srpt_inst.is_timed = 0;
}

void srpt_start_time(SRPT_instance srpt_inst) {
	srpt_inst.is_timed 	= 1;
	srpt_inst.time_elpsd = 0.0;
	srpt_inst.start 		= clock();
	srpt_inst.diff 		= srpt_inst.start;
}

void srpt_update_timer(SRPT_instance srpt_inst) {
	if (srpt_inst.is_timed) {
		srpt_inst.diff = clock() - srpt_inst.start;
		// srpt_inst.time_elpsd = (double) srpt_inst.diff

		//add timeout detection here
	}
}

#endif
