/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 15 Nov 2015

Header file for datalink definitions
*/

#ifndef RDT_DATALINK_H_
#define RDT_DATALINK_H_

void datalinkInit(char * protocol);
int datalinkSend(int c_sockfd, AppMessage msg);

#endif