/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Datalink layer commands file.
*/
#include "config.h"

Frame make_fram(bool isServer, Packet data) {
	Frame f;
	return f;
}

/*
	called from above (application layer)
	-converts app data to datalink data (packet - > frame)
*/
void rdt_send(bool isServer, Packet data) {
	Frame frame;
	frame.pkt = data;
	udt_send(isServer, data);
}

/*
	called by rdt
	-sends Frame over to physical layer to be send via udt
*/
void udt_send(bool isServer, Frame data) {
	if (isServer) {
		// extract socketfd and forward to given socket
	}
	else {
		// client sending
	}
}

/*
	called by udt
*/
void rdt_recv(int sockfd, Frame data) {
	data.pkt.sockfd = sockfd;
}
/*
	called by rdt (data link layer)
	-delivers data to app layer (frame -> packet)
*/
void deliver_data(Packet data) {
	int sockfd = data.pkt.sockfd;
} 

bool corrupt_pkt(Packet data) {
	return false;
}