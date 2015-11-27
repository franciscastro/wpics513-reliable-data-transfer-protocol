# Makefile for compiling server and client code
# Last updated: 27 November 2015

all:
	gcc -o tcrserver tcr_server.c -lpthread
	gcc -o client rdt_client.c -lpthread