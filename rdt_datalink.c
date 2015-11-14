/*
Authors: Francisco Castro, Antonio Umali
CS 513 Project 2 - Reliable Data Transfer Protocol
Last modified: 14 Nov 2015

Datalink definition file
*/

void datalink_init(char * protocol, int windowSize, double lossRate, double corruptionRate) {
    if (strcmp(protocol, "gbn") == 0) {
    	gbn_init(windowSize, lossRate, corruptionRate);
    	g_datalink.send = &gbn_send;
    	g_datalink.recv = &gbn_recv;
    } else if (strcmp(protocol, "sr") == 0) {
    	sr_init(windowSize, lossRate, corruptionRate);
    	g_datalink.send = &sr_send;
    	g_datalink.recv = &sr_recv;
    } else {
    	printf("Abort: Unrecognized transfer protocol. \n");
    	exit(1);
    }
    printf("protocol = %s, window_size = %d, loss_rate = %lf corruption_rate = %lf\n",
    			protocol, windowSize, lossRate, corruptionRate);
}