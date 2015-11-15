# Makefile for compiling project
# Last updated: 15 November 2015

all:
	rm *.o
	gcc -g -Iinclude -c rdt_client.c -o ./client.o
	gcc -g -Iinclude -c client_lib.c -o ./client_lib.o
	gcc -g -Iinclude -o client client.o client_lib.o -lpthread