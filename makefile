# Makefile for compiling server and client code
# Last updated: 12 October 2015

all:
	gcc -o checksum_test checksum_test.c -lpthread -w 
	gcc -o rdt_client rdt_client.c -lpthread -w 
	gcc -o rdt_server rdt_server.c -lpthread -w 