======================================================================================

Authors: Francisco Castro and Antonio Umali
Project: CS 513 Project 1 - Text ChatRoulette
Description: README file for the Text ChatRoulette project
Last modified: 12 October 2015

======================================================================================

Notes:
- This project was developed, compiled, and tested on an Ubuntu Linux OS machine
- These instructions assume the use of a Linux machine

======================================================================================

This is the README file for the Text ChatRoulette project. Read this prior to compiling and running the project. This file contains information on the following:

[1] Project files
[2] Compiling Text ChatRoulette
[3] Running Text ChatRoulette

======================================================================================

[1] PROJECT FILES

Before compiling and running Text ChatRoulette, make sure the following files are present in a single directory:

1. Server files
(a) tcr_server.c
(b) tcr_server.h

2. Client files
(a) tcr_client.c
(b) tcr_client.h

3. Other project files
(a) makefile
(b) HOSTNAME
(c) README.txt (this file)

If the project is compressed as an archive file, extract the contents of the archive into a single directory.

======================================================================================

[2] COMPILING TEXT CHATROULETTE

a. Set the hostname of the Text ChatRoulette server in the HOSTNAME file. You may edit the hostname indicated in the file with a text editor. Save and close the HOSTNAME file.

b. Once you have all the necessary files listed in [1], open a terminal, navigate to the directory containing the project files and run the following command:

make

c. Running the make command will produce two (2) executable files:
- tcrserver
- tcrclient


======================================================================================

[3] RUNNING TEXT CHATROULETTE

a. To run the Text ChatRoulette server, open a terminal, navigate to the directory containing the project files and the executable files produced by 'make', and run the following command:

./tcrserver

b. To run the Text ChatRoulette client, open a terminal, navigate to the directory containing the project files and the executable files produced by 'make', and run the following command:

./tcrclient

c. You may run multiple instances of the client to connect to the server. Just copy the directory containing all the project files and executable files into another directory and run the client from there.

======================================================================================
