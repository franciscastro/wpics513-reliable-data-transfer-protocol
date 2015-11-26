/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Go-Back-N ARQ file.
*/

#include "config.h";
#include "data_link.h"

typedef struct GBN_instance {
	bool isServer;
    double time_elpsd;
    int window_size;
    int base;
    int next_seq_num;
    int buffer_size;
    bool timed;
    Packet packet_buffer[];
} GBN_instance;

void gbn_init(GBN_instance gb_inst) {

}

void gbn_recv(GBN_instance gb_inst) {
	
	// single state FSM
	while(1) {

	}
}

void gbn_rdt_send(GBN_instance gb_inst, Packet pkt) {
	if (gb_inst.next_seq_num < gb_inst.base + gb_inst.window_size) {
		rdt_send(gb_inst.isServer, pkt);
		if (gb_inst.base == gb_inst.next_seq_num) {
			gbn_start_time(gb_inst);
			gb_inst.next_seq_num += 1;
		}
	}
}

void gbn_rdt_rcv(GBN_instance gb_inst) {
	
}

void gbn_timeout(GBN_instance gb_inst) {
	gbn_start_time(gb_inst);
	for (int i = 0; i < gb_inst.buffer_size; ++i) {
		rdt_send(gb_inst.isServer, gb_inst.packet_buffer[i]);
	}
}

void gbn_start_time(GBN_instance gb_inst) {
	gb_inst.time_elpsd = 0.0;
}

void gbn_update_timer(GBN_instance gb_inst) {
	
}

void gbn_refuse_data(GBN_instance gb_inst) {

}