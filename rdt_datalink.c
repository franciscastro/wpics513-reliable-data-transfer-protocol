/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 15 Nov 2015

Datalink file
*/

#include "config.h"
#include "rdt_datalink.h"

int transferProtocol;

void datalinkInit(char * protocol) {

    // Transfer protocol is Go-back-N
    if (strcmp(protocol, "gbn") == 0) {
        transferProtocol = GOBACKN;

    } 
    // Transfer protocol is Selective-repeat
    else if (strcmp(protocol, "sr") == 0) {
        transferProtocol = SELREPEAT;
    }
}

int datalinkSend(int c_sockfd, AppMessage msg) {
    return 1;
}