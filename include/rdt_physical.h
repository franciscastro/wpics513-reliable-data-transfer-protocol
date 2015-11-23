/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 21 Nov 2015

Header file for physical layer definitions
*/

#ifndef RDT_PHYSICAL_H_
#define RDT_PHYSICAL_H_

void physicalInit(char * protocol, int corruptRate, int dropRate);

#endif