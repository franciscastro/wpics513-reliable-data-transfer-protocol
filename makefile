# Makefile for compiling server and client code
# Last updated: 12 October 2015

all:
	gcc -o tcrserver tcr_server.c -lpthread
	gcc -o tcrclient tcr_client.c -lpthread