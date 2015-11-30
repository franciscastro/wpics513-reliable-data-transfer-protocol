/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 23 Oct 2015

Datalink layer commands file.
*/
#ifndef DATALINK_H_
#define DATALINK_H_

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "config.h"

typedef struct FrameBuffer{
	FrameBufferEntry * head;
	FrameBufferEntry * tail;
	int count;
}FrameBuffer;

typedef struct PacketBuffer{
	BufferEntry * head;
	BufferEntry * tail;
	int count
}PacketBuffer;

int droprate;
int corruptrate;

FrameBuffer client_rcv_buffer;
FrameBuffer client1_rcv_buffer;
FrameBuffer client2_rcv_buffer;

FrameBuffer client_snd_buffer;
FrameBuffer client1_snd_buffer;
FrameBuffer client2_snd_buffer;

PacketBuffer client_pkt_rcv_buffer;
PacketBuffer client1_pkt_rcv_buffer;
PacketBuffer client2_pkt_rcv_buffer;

PacketBuffer client_pkt_snd_buffer;
PacketBuffer client1_pkt_snd_buffer;
PacketBuffer client2_pkt_snd_buffer;

int client1_sockfd;
int client2_sockfd;


time_t t;

/* Intializes random number generator */

void push_frame_rcv_buffer(int is_srvr, int socketfd, FrameBufferEntry * fbe) {
	// if (fbe->frame.type != ACK_F)
	// 	printf("Pushing into frame rcv buffer\n");
	fbe->next = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {

			FrameBufferEntry * ite = NULL;
			if (client1_rcv_buffer.head != NULL) {
				ite = client1_rcv_buffer.head;
				while (ite != NULL) {
					if (ite->frame.seqNumber == fbe->frame.seqNumber)
						return;
				}
			}

			if (client1_rcv_buffer.head == NULL) {
				client1_rcv_buffer.head = fbe;
				client1_rcv_buffer.tail = fbe;
			}
			else {
				client1_rcv_buffer.tail->next = fbe;
				client1_rcv_buffer.tail = fbe;
			}
		}
		else if(socketfd == client2_sockfd) {

			FrameBufferEntry * ite = NULL;
			if (client2_rcv_buffer.head != NULL) {
				ite = client2_rcv_buffer.head;
				while (ite != NULL) {
					if (ite->frame.seqNumber == fbe->frame.seqNumber)
						return;
				}
			}

			if (client2_rcv_buffer.head == NULL) {
				client2_rcv_buffer.head = fbe;
				client2_rcv_buffer.tail = fbe;
			}
			else {
				client2_rcv_buffer.tail->next = fbe;
				client2_rcv_buffer.tail = fbe;
			}
		}
	}
	else {
		FrameBufferEntry * ite = NULL;
		if (client_rcv_buffer.head != NULL) {
			ite = client_rcv_buffer.head;
			while (ite != NULL) {
				if (ite->frame.seqNumber == fbe->frame.seqNumber)
					return;
			}
		}

		if (client_rcv_buffer.head == NULL) {
			client_rcv_buffer.head = fbe;
			client_rcv_buffer.tail = fbe;
		}
		else {
			client_rcv_buffer.tail->next = fbe;
			client_rcv_buffer.tail = fbe;
		}
	}
	// if (fbe->frame.type != ACK_F) {
	// 	printf("Pushing a new frame buffer entry for frame\n");
	// 	printf("With seqNumber %d\n", fbe->frame.seqNumber);
	// }
}

FrameBufferEntry * pop_frame_rcv_buffer(int is_srvr, int is_sndr, int socketfd) {
	FrameBufferEntry * ret = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_rcv_buffer.head == NULL)
				return NULL;
			if ((is_sndr && client1_rcv_buffer.head->frame.type == ACK_F) ||
				(!is_sndr && client1_rcv_buffer.head->frame.type == DATA_F)) {
				ret = client1_rcv_buffer.head;
				client1_rcv_buffer.head = ret->next;
				return ret;
			}
			else
				return NULL;
		}
		else if (socketfd == client2_sockfd) {
			if (client2_rcv_buffer.head == NULL)
				return NULL;
			if ((is_sndr && client2_rcv_buffer.head->frame.type == ACK_F) ||
				(!is_sndr && client2_rcv_buffer.head->frame.type == DATA_F)) {
				ret = client2_rcv_buffer.head;
				client2_rcv_buffer.head = ret->next;
				return ret;
			}
			else
				return NULL;
		}
		else 
			return NULL;
	}
	else {
		if (client_rcv_buffer.head == NULL)
			return NULL;

		if ((is_sndr && client_rcv_buffer.head->frame.type == ACK_F) ||
			(!is_sndr && client_rcv_buffer.head->frame.type == DATA_F)) {
			ret = client_rcv_buffer.head;
			client_rcv_buffer.head = ret->next;
			return ret;
		}
		else
			return NULL;
	}
}

void push_frame_snd_buffer(int is_srvr, int socketfd, FrameBufferEntry * fbe) {
	fbe->next = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_snd_buffer.head == NULL) {
				client1_snd_buffer.head = fbe;
				client1_snd_buffer.tail = fbe;
			}
			else {
				client1_snd_buffer.tail->next = fbe;
				fbe->prev = client1_snd_buffer.tail;
				client1_snd_buffer.tail = fbe;
			}
		}
		else if(socketfd == client2_sockfd) {
			if (client2_snd_buffer.head == NULL) {
				client2_snd_buffer.head = fbe;
				client2_snd_buffer.tail = fbe;
			}
			else {
				client2_snd_buffer.tail->next = fbe;
				fbe->prev = client2_snd_buffer.tail;
				client2_snd_buffer.tail = fbe;
			}
		}
	}
	else {
		if (client_snd_buffer.head == NULL) {
			client_snd_buffer.head = fbe;
			client_snd_buffer.tail = fbe;
		}
		else {
			client_snd_buffer.tail->next = fbe;
			fbe->prev = client_snd_buffer.tail;
			client_snd_buffer.tail = fbe;
		}
	}
}

FrameBufferEntry * pop_frame_snd_buffer(int is_srvr, int socketfd) {
	FrameBufferEntry * ret = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_snd_buffer.head == NULL)
				return NULL;
			printf("SERVER: client1 send buffer has something!\n");
			ret = client1_snd_buffer.head;
			client1_snd_buffer.head = ret->next;
			return ret;
		}
		else if (socketfd == client2_sockfd) {
			if (client2_snd_buffer.head == NULL)
				return NULL;
			printf("SERVER: client2 send buffer has something!\n");
			ret = client2_snd_buffer.head;
			client2_snd_buffer.head = ret->next;
			return ret;
		}
		else 
			return NULL;
	}
	else {
		if (client_snd_buffer.head == NULL)
			return NULL;
		printf("CLIENT: client side send buffer has something!\n");
		ret = client_snd_buffer.head;
		client_snd_buffer.head = ret->next;
		return ret;
	}
}

FrameBuffer * get_from_frame_snd_buffer(int is_srvr, int socketfd, int seq_num) {
	FrameBufferEntry * ret = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_snd_buffer.head == NULL)
				return NULL;
			ret = client1_snd_buffer.head;
		}
		else if (socketfd == client2_sockfd) {
			if (client2_snd_buffer.head == NULL)
				return NULL;
			ret = client2_snd_buffer.head;
		}
		else 
			return NULL;
	}
	else {
		if (client_snd_buffer.head == NULL)
			return NULL;
		ret = client_snd_buffer.head;
	}

	while (ret != NULL && ret->frame.seqNumber != seq_num) {
		ret = ret->next;
	}
	return ret;
}

void remove_from_frame_snd_buffer(int is_srvr, int socketfd, int seqNumber) {
	FrameBufferEntry * ret = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_snd_buffer.head != NULL)
				ret = client1_snd_buffer.head;
		}
		else if (socketfd == client2_sockfd) {
			if (client2_snd_buffer.head != NULL)
				ret = client2_snd_buffer.head;
		}
	}
	else {
		if (client_snd_buffer.head != NULL)
			ret = client_snd_buffer.head;
	}

	while (ret != NULL &&ret->frame.seqNumber != seqNumber) {
		ret = ret->next;
	}

	if (ret != NULL) {
		ret->prev->next = ret->next;
		ret->next->prev = ret->prev;

		free(ret);
	}
}

void push_pkt_rcv_buffer(int is_srvr, int socketfd, BufferEntry * pbe) {
	pbe->next = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_pkt_rcv_buffer.head == NULL) {
				client1_pkt_rcv_buffer.head = pbe;
				client1_pkt_rcv_buffer.tail = pbe;
			}
			else {
				client1_pkt_rcv_buffer.tail->next = pbe;
				client1_pkt_rcv_buffer.tail = pbe;
			}
		}
		else if(socketfd == client2_sockfd) {
			if (client2_pkt_rcv_buffer.head == NULL) {
				client2_pkt_rcv_buffer.head = pbe;
				client2_pkt_rcv_buffer.tail = pbe;
			}
			else {
				client2_pkt_rcv_buffer.tail->next = pbe;
				client2_pkt_rcv_buffer.tail = pbe;
			}
		}
	}
	else {
		if (client_pkt_rcv_buffer.head == NULL) {
			client_pkt_rcv_buffer.head = pbe;
			client_pkt_rcv_buffer.tail = pbe;
		}
		else {
			client_pkt_rcv_buffer.tail->next = pbe;
			client_pkt_rcv_buffer.tail = pbe;
		}
	}
}

BufferEntry * pop_pkt_rcv_buffer(int is_srvr, int socketfd) {
	BufferEntry * ret = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_pkt_rcv_buffer.head == NULL)
				return NULL;
			ret = client1_pkt_rcv_buffer.head;
			client1_pkt_rcv_buffer.head = ret->next;
			return ret;
		}
		else if (socketfd == client2_sockfd) {
			if (client2_pkt_rcv_buffer.head == NULL)
				return NULL;
			ret = client2_pkt_rcv_buffer.head;
			client2_pkt_rcv_buffer.head = ret->next;
			return ret;
		}
		else 
			return NULL;
	}
	else {
		if (client_pkt_rcv_buffer.head == NULL)
			return NULL;
		ret = client_pkt_rcv_buffer.head;
		client_pkt_rcv_buffer.head = ret->next;
		return ret;
	}
}

void push_pkt_snd_buffer(int is_srvr, int socketfd, BufferEntry * pbe) {
	pbe->next = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_pkt_snd_buffer.head == NULL) {
				client1_pkt_snd_buffer.head = pbe;
				client1_pkt_snd_buffer.tail = pbe;
			}
			else {
				client1_pkt_snd_buffer.tail->next = pbe;
				client1_pkt_snd_buffer.tail = pbe;
			}
		}
		else if(socketfd == client2_sockfd) {
			if (client2_pkt_snd_buffer.head == NULL) {
				client2_pkt_snd_buffer.head = pbe;
				client2_pkt_snd_buffer.tail = pbe;
			}
			else {
				client2_pkt_snd_buffer.tail->next = pbe;
				client2_pkt_snd_buffer.tail = pbe;
			}
		}
	}
	else {
		if (client_pkt_snd_buffer.head == NULL) {
			client_pkt_snd_buffer.head = pbe;
			client_pkt_snd_buffer.tail = pbe;
		}
		else {
			client_pkt_snd_buffer.tail->next = pbe;
			client_pkt_snd_buffer.tail = pbe;
		}
	}
}

BufferEntry * pop_pkt_snd_buffer(int is_srvr, int socketfd) {
	BufferEntry * ret = NULL;
	if (is_srvr) {
		if (socketfd == client1_sockfd) {
			if (client1_pkt_snd_buffer.head == NULL)
				return NULL;
			ret = client1_pkt_snd_buffer.head;
			client1_pkt_snd_buffer.head = ret->next;
			return ret;
		}
		else if (socketfd == client2_sockfd) {
			if (client2_pkt_snd_buffer.head == NULL)
				return NULL;
			ret = client2_pkt_snd_buffer.head;
			client2_pkt_snd_buffer.head = ret->next;
			return ret;
		}
		else 
			return NULL;
	}
	else {
		if (client_pkt_snd_buffer.head == NULL)
			return NULL;
		ret = client_pkt_snd_buffer.head;
		client_pkt_snd_buffer.head = ret->next;
		return ret;
	}
}

int udt_send_frame(int is_srvr, int socket_to, Frame * frame) {

	// generate random number for drop and corrupt
	int drop, corrupt;
	drop = 1 + (rand() % 100);
	corrupt = 1 + (rand() % 100);

	// if (frame->type != ACK_F) {
	// 	printf("udt_send_frame to socket: %d\n with frame message type %d\n and seqNumber %d\n", socket_to, frame->pkt.msgType, frame->seqNumber);
	// 	printf("checksum is %s\n", frame->checksum);
	// }

	if (socket_to < 0)
		return 1;
	
	int framelen = sizeof(Frame);
	int total = 0;				// How many bytes we've sent
    int bytesleft = framelen;	// How many we have left to send
    int n;

    // To make sure all data is sent
	// if (frame->type != ACK_F) 
	//     printf("sending %d bytes from size %d, is null? %d\n", framelen, sizeof(frame), frame == NULL);
	
	// if (is_srvr == 0)
	// 	printf("drop is %d\n", drop);
	if (is_srvr == 1 || drop < 20) {
	    while(total < framelen) {

	        n = send(socket_to, (frame + total), bytesleft, 0);
			if (frame->type != ACK_F) 
		        // 	printf("sent n = %d\n", n);
	        
	        if (n == -1) { break; }
	        
	        total += n;
	        bytesleft -= n;

	    }	
	}

    framelen = total;	// Return number actually sent here

    n = 188;
	return n == -1 ? -1 : 0;	// Return 0 on success, -1 on failure
}

void free_frame(Frame * frame) {
	memset(frame, 0, sizeof(Frame)); // don't do this
}

// checksum utility functions
/*
	returns the byte representation (as a char array) of a source character
*/
unsigned char * return_byte (unsigned char source)
{
    unsigned char *p = malloc(CHAR_BIT * sizeof(unsigned char));
    int i;

    for(i = 0; i < CHAR_BIT; ++i)
        p[i] = '0' + ((source & (1 << i)) > 0);

    return p;
}
/*
	Functions which does binary addition on two 8-bit binary numbers represented
	by two char arrays
*/
unsigned char * add_bytes(unsigned char* a, unsigned char* b) {
	int i;
	unsigned char * sum = malloc(CHAR_BIT * sizeof(unsigned char));
	unsigned char * carry = malloc(CHAR_BIT * sizeof(unsigned char));

	for (i = 0; i < CHAR_BIT; ++i) {
		sum[i] = '0';
		carry[i] = '0';
	}

	for (i = 0; i < CHAR_BIT; ++i) {
		if (a[i] == '1' && b[i] == '1') {
			if (i == 0) {
				carry[i] = '1';
				sum[i] = '0';
			}
			else {
				if (carry[i-1] == '1') {
					sum[i] = '1';
					carry[i] = '1';
				}
				else if(carry[i-1] == '0') {
					carry[i] = '1';
					sum[i] = '0';
				}
			}
		}
		else if ((a[i] == '1' && b[i] == '0') || (a[i] == '0' && b[i] == '1')) {
			if (i == 0) {
				sum[i] = '1';
				carry[i] = '0';
			}
			else {
				if(carry[i-1] == '1') {
					sum[i] = '0';
					carry[i] = '1';
				}
				else {
					sum[i] = '1';
					carry[i] = '0';
				}
			}
		}
		else { //both are 0
			if (i == 0) {
				carry[i] = '0';
				sum[i] = '0';
			}
			else {
				if (carry[i-1] == '1') {
					carry[i] = '0';
					sum[i] = '1';
				}
				else {
					carry[i] = '0';
					sum[i] = '0';
				}
			}
		}
	}
	free(carry);
	return sum;
}

/*
	Function which negates the bits of an 8-bit binary number which is 
	represented as a char array
*/
unsigned char* negate_byte(unsigned char* a) {
	unsigned char * ret = malloc(CHAR_BIT * sizeof(unsigned char));
	int i;
	for (i = 0; i < CHAR_BIT; ++i) {
		if (a[i] == '1') 
			ret[i] = '0';
		else 
			ret[i] = '1';
	}
	return ret;
}

/*
	Function which adds one bit to the given 8-bit binary number which is
	represented as a char array
*/
unsigned char* add_one_bit(unsigned char* a) {
	unsigned int one = 1;
	unsigned char* one_char = (unsigned char*)&one;

	return add_bytes(return_byte(one_char[0]), a);
}

/*
	Function which recovers the original checksum given its two's complement.
	Both are 8-bit binary numbers represented as a char array;
*/
unsigned char* recover_checksum(unsigned char* checksum) {
	unsigned char * ret = malloc(CHAR_BIT * sizeof(unsigned char));
	int i = 0;
	while (checksum[i] == '0' && i < CHAR_BIT) {
		ret[i] = '1';
		++i;
	}
	ret[i] = '0';
	++i;
	while (i < CHAR_BIT) {
		ret[i] = checksum[i];
		++i;
	}
	return negate_byte(ret);
}

unsigned char* compute_checksum(Frame frame) {
	char* sockfd_byte = (char*)&frame.pkt.sockfd;
	char* sq_nmbr_byte = (char*)&frame.seqNumber;
	char* nxt_ack_byte = (char*)&frame.nextAckExpected;
	char* frm_type_byte = (char*)&frame.type;
	char* msg_type_byte = (char*)&frame.pkt.msgType;

 	unsigned int total_frame_size = 180; //180 bytes

 	int i, idx = 0;
	unsigned char * chk_sum = malloc(CHAR_BIT * sizeof(unsigned char));
	for (i = 0; i < CHAR_BIT; ++i)
		chk_sum[i] = '0';

	unsigned char val;
    unsigned char * val_byte;
 	for (i = 0; i < total_frame_size; ++i) {
		if(i < 4) { // Frame seq number
	    	val = sq_nmbr_byte[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 3)
					idx = 0;
				else
					++idx;
		}
		else if(i < 8) { // Frame next ack expected
	    	val = nxt_ack_byte[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 7)
					idx = 0;
				else
					++idx;
		}
		else if(i < 12) {// Frame type
	    	val = frm_type_byte[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 11)
					idx = 0;
				else
					++idx;
		}
		else if(i < 16) { // sockfd
	    	val = sockfd_byte[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 15)
					idx = 0;
				else
					++idx;
		}
		else if(i < 20) {// msg type
	    	val = msg_type_byte[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 15)
					idx = 0;
				else
					++idx;
		}
		else if(i < 20 + ALIASSIZE) {
	    	val = frame.pkt.alias[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 20 + ALIASSIZE + FILENAMESIZE + MAXDATA - 1)
					idx = 0;
				else
					++idx;
		}
		else if(i < 20 + ALIASSIZE + FILENAMESIZE) {
	    	val = frame.pkt.filename[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 20 + ALIASSIZE + FILENAMESIZE - 1)
					idx = 0;
				else
					++idx;
		}
		else if(i < 20 + ALIASSIZE + FILENAMESIZE + MAXDATA) {
	    	val = frame.pkt.data[idx];
		    val_byte = return_byte(val);
	    	chk_sum = add_bytes(chk_sum, val_byte);
				if (i == 20 + ALIASSIZE + FILENAMESIZE + MAXDATA - 1)
					idx = 0;
				else
					++idx;
		}
 	}
 	return chk_sum;
}

Frame make_frame(int is_srvr, Packet data, FrameType type, int sq_num, int next_ack) {
	Frame f;
	f.pkt = data;
	f.type = type;
	f.seqNumber = sq_num;

	char * checksum = compute_checksum(f);
	int c;
	for (c = 0; c < CHAR_BIT; ++c)
		f.checksum[c] = checksum[c];
	// strcpy(f.checksum, compute_checksum(f));
	return f;
}

/*
	called from above (application layer)
	-converts app data to datalink data (packet - > frame)
*/
void rdt_send(int is_srvr, Packet data) {
	Frame frame;
	frame.type = DATA_F;
	frame.pkt  = data;
	udt_send(is_srvr, frame);
}

/*
	called by rdt
	-sends Frame over to physical layer to be send via udt
*/
void udt_send(int is_srvr, Frame data) {
	if (is_srvr) {
		// extract socketfd and forward to given socket
		int sockfd = data.pkt.sockfd;
		udt_send_frame(is_srvr, sockfd, &data);
	}
	else {
		// client sending
		int sockfd = data.pkt.sockfd;
		udt_send_frame(is_srvr, sockfd, &data);
	}
}

int corrupt_frame(Frame data) {
	unsigned char * data_check_sum = compute_checksum(data);
	
	int i;
	for (i = 0; i < 8; ++i) {
		if (data_check_sum[i] != data.checksum[i])
			return true;
	}

	return false;
}

/*
	called by udt
*/
void rdt_recv(int is_srvr, int sockfd, Frame data) {
	data.pkt.sockfd = sockfd;

	if (corrupt_frame(data))
		refuse_data(is_srvr, data);
}
/*
	called by rdt (data link layer)
	-delivers data to app layer (frame -> packet)
*/
void deliver_data(int is_srvr, int client_sockfd, BufferEntry * pbe) {
	push_pkt_rcv_buffer(is_srvr, client_sockfd, pbe);
}

Packet * extract_data(int is_srvr, int client_sockfd) {
	BufferEntry * pbe = pop_pkt_rcv_buffer(is_srvr, client_sockfd);
	// get frame from buffer
	if (pbe != NULL) //if buffer not empty
		return &(pbe->pkt);
	else
		return NULL;
} 

void refuse_data(int is_srvr, Frame frame) {
	Frame ref_frame = make_frame(is_srvr, frame.pkt, ACK_F, frame.seqNumber, frame.nextAckExpected); //make a refusal frame here
	udt_send(is_srvr, ref_frame);
}
#endif