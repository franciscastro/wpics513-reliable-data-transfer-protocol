======================================================================================

Authors: Francisco Castro and Antonio Umali
Project: CS 513 Project 2 - Reliable Data Transfer Protocol (RDT): Echo Server
Description: README file for the RDT: Echo Server project
Last modified: 29 November 2015

======================================================================================

Notes:
- This project was developed, compiled, and tested on an Ubuntu Linux OS machine
- These instructions assume the use of a Linux machine

======================================================================================

This is the README file for the RDT: Echo Server project. Read this prior to compiling and running the project. This file contains information on the following:

[1] Project files
[2] Compiling RDT: Echo Server
[3] Running RDT: Echo Server

======================================================================================

[1] PROJECT FILES

Before compiling and running RDT: Echo Server, make sure the following files are present in a single directory:
gbn datalink srpt
1. Server files
(a) rdt_server.c
(b) rdt_server.h

2. Client files
(a) rdt_client.c
(b) rdt_client.h

3. Additional header files
(a) datalink.h
(b) gbn.h
(c) srpt.h

3. Other project files
(a) makefile
(b) README.txt (this file)
(c) stat-graphs.xlsx
(d) stat-graphs.pdf

If the project is compressed as an archive file, extract the contents of the archive into a single directory.

======================================================================================

[2] COMPILING RDT: ECHO SERVER

a. Set the hostname of the RDT: Echo Server in the config.h file. You may edit the hostname indicated in the file with a text editor. Save and close the config.h file.

b. Once you have all the necessary files listed in [1], open a terminal, navigate to the directory containing the project files and run the following command:

make all

c. Running the make command will produce two (2) executable files:
- rdtserver
- rdtclient


======================================================================================

[3] RUNNING RELIABLE DATA TRANSFER PROTOCOL: ECHO SERVER

a. To run the RDT: Echo Server server, open a terminal, navigate to the directory containing the project files and the executable files produced by 'make all', and run the following command:

./rdtserver [gbn|sr] [corrupt rate] [drop rate]

b. To run the RDT: Echo Server client, open a terminal, navigate to the directory containing the project files and the executable files produced by 'make', and run the following command:

./rdtclient [gbn|sr] [corrupt rate] [drop rate]

c. It is important to note that the transfer protocol selected in the server must be the same as the client when running the program.

======================================================================================
