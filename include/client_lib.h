/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 15 Nov 2015

Header file for client definitions
*/

#ifndef CLIENT_LIB_H
#define CLIENT_LIB_H

char* removeSpace(char *s);
void *get_in_addr(struct sockaddr *sa);
void allCaps(char *command);
int connectToServer();
void receivedDataHandler(struct packet *msgrecvd);
void help();

#endif