/*
* File : reciever.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include "dcomm.h"

/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 8

Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
Boolean send_xon = false, send_xoff = false;

/* Socket */
int sockfd; // listen of sock_fd

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);

int main(int argc, char const *argv[])
{
	/* code */
	Byte c;
	/*
		Insert code here to bind socket to the port number given in argv[1].
	*/

	/* Initialize XON/XOFF flags */
	/* Create child process */
	
	/*** IF PARENT PROCESS ***/
	while ( true ) {
		c = *(rcvchar(sockfd, rxq));
		/* Quit on end of file */
		if (c == Endfile) {
			exit(0);
		}
	}

	/*** ELSE IF CHILD PROCESS ***/
		while ( true ) {
		/* Call q_get */
		/* Can introduce some delay here. */
		}
	return 0;
}

static Byte *rcvchar(int sockfd, QTYPE *queue) {
	/*
	Insert code here.
	Read a character from socket and put it to the receive
	buffer.
	If the number of characters in the receive buffer is above
	certain level, then send XOFF and set a flag (why?).
	Return a pointer to the buffer where data is put.
	*/
}
	/* q_get returns a pointer to the buffer where data is read or NULL if
	* buffer is empty.
	*/

static Byte *q_get(QTYPE *queue, Byte *data) {
	Byte *current;
	/* Nothing in the queue */
	
	if (!queue->count) return (NULL);
	/*
	Insert code here.
	Retrieve data from buffer, save it to "current" and "data"
	If the number of characters in the receive buffer is below
	certain level, then send XON.
	Increment front index and check for wraparound.
	*/
}