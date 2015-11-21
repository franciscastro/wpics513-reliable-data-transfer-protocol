# Makefile for compiling project
# Last updated: 21 November 2015

all:
	gcc -g -Iinclude -c rdt_client.c -o ./client.o
	gcc -g -Iinclude -c client_lib.c -o ./client_lib.o
	gcc -g -Iinclude -c rdt_datalink.c -o ./datalink.o
	gcc -g -Iinclude -c rdt_gbn.c -o ./gbn.o
	gcc -g -Iinclude -o client client.o client_lib.o datalink.o gbn.o -lpthread