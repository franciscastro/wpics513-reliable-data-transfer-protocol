/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Go-Back-N ARQ file.
*/

#include "config.h";
#include 

typedef struct GBN_instance {
    double time_elpsd;
    int window_size;
    int base;
    int next_seq_num;
} GBN_instance;

void gbn_init(GBN_instance gb_inst) {

}

void gbn_recv(GBN_instance gb_inst) {
	
	// single state FSM
	while(1) {

	}
}

void gbn_rdt_send(GBN_instance gb_inst) {

}

void gbn_rdt_rcv(GBN_instance gb_inst) {
	
}

void gbn_timeout(GBN_instance gb_inst) {

}

void gbn_refuse_data(GBN_instance gb_inst) {

}