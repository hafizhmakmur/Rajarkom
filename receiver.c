/*
* File : receiver.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include <netdb.h>
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



/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(int sockfd, QTYPE *, Byte *);

/* Other variables */
Byte t[2];
int rcvdByte = 0;
int cnsmByte = 0;
struct sockaddr_in client, local;

#define MIN_UPPERLIMIT 4
#define MAX_LOWERLIMIT 1


void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char const *argv[])
{
	/* code */
	Byte c;
	
	/* Socket */
	int sockfd; // listen of sock_fd
	
	/*
		Insert code here to bind socket to the port number given in argv[1].
	*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Socket gagal\n");
		exit(0);
	}

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		error("setsockopt(SO_REUSEADDR) failed");

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(atoi(argv[1]));
	if (bind(sockfd, (struct sockaddr*)&local, sizeof(local)) != 0){
		printf("Binding gagal\n");
		exit(0);
	}
	else{
		printf("Binding pada %d.%d.%d.%d:%d\n", (int)(local.sin_addr.s_addr & 0xFF), (int)((local.sin_addr.s_addr & 0xFF00) >> 8), (int)((local.sin_addr.s_addr & 0xFF0000) >> 16), (int)((local.sin_addr.s_addr & 0xFF000000) >> 24), (int)ntohs(local.sin_port));
	}
	if(listen(sockfd, 5) != 0){
		printf("Gagal mendengar\n");
	}
	/* Initialize XON/XOFF flags */
	
	/* Create child process */
	printf("There there\n");

    char buf[256];    
    while(1) {
		 
		// Wait until client connect
		int clilen = sizeof(client);
		int newsockfd = accept(sockfd, (struct sockaddr*)&client, &clilen);

		if (newsockfd < 0) error("ERROR on accept");
	
		printf("Here I am\n");
		pid_t  pid = fork();
		if(pid != 0){
			printf("Parent here %d\n",pid);
			/*** IF PARENT PROCESS ***/
			while ( true ) {
				c = *(rcvchar(newsockfd, rxq));
		
				/* Quit on end of file */	
				if (c == Endfile) {
					exit(0);
				}
			}
		} else {
		
			/*** ELSE IF CHILD PROCESS ***/
		
			while ( true ) {
				/* Call q_get */
				Byte *coba = q_get(newsockfd, rxq, &c);
				if(coba != NULL){
					if(rxq->front > 0){
						if((rxq->data[rxq->front - 1] != Endfile) && (rxq->data[rxq->front - 1] != CR) && (rxq->data[rxq->front - 1] != LF)){
							printf("Mengkonsumsi byte ke-%d: '%c'\n", ++cnsmByte, rxq->data[rxq->front - 1]);
						}
						else if(rxq->data[rxq->front - 1] == Endfile){
							//do nothing
							exit(0);
						}
					}
					else{
						if((rxq->data[7] != Endfile) && (rxq->data[7] != CR) && (rxq->data[7] != LF)){
							printf("Mengkonsumsi byte ke-%d: '%c'\n", ++cnsmByte, rxq->data[7]);
						}
						else if(rxq->data[7] == Endfile){
							exit(0);
						}
					}
					
				}
				
				/* Can introduce some delay here. */
				usleep(DELAY * 1000);
			}
		}
		printf("Try again\n");
	}
	printf("Game over\n");
	close(sockfd);
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
	
	if(!send_xoff){
		printf("Greetings\n");
		int clilen = sizeof(client);
		ssize_t nBytesRcvd = recvfrom(sockfd, t, sizeof(t), 0, (struct sockaddr*)&client, &clilen);
		printf("How's there\n");
		if(nBytesRcvd < 0){
			printf("recvfrom failed\n");
		}
		else{
			queue->data[queue->rear] = t[0];
			queue->count = queue->count + 1;
			if(queue->rear < 7){
				queue->rear = queue->rear + 1;
			}
			else{
				queue->rear = 0;
			}
			rcvdByte = rcvdByte + 1;
			printf("Pass this\n");
		}
		if((t[0] != Endfile) && (t[0] != CR) && (t[0] != LF)){
			printf("Menerima byte ke-%d\n", rcvdByte);
		}
		
		if((queue->count > MIN_UPPERLIMIT) && (sent_xonxoff == XON)){
			sent_xonxoff = XOFF;
			send_xoff = true;
			send_xon = false;
			printf("Buffer > minimum upperlimit\n");
			char test[2];
			test[0] = XOFF;
			ssize_t nBytesSnt = sendto(sockfd, test, sizeof(test), 4, (struct sockaddr*)&client, sizeof(client));
			printf("Mengirim XOFF\n");
			if(nBytesSnt < 0){
				printf("sendto failed\n");
			}
		}
		return &t[0];
	}
	else{
		Byte *buff = 0;
		return buff;
	}
}

	/* q_get returns a pointer to the buffer where data is read or NULL if
	* buffer is empty.
	*/

static Byte *q_get(int sockfd, QTYPE *queue, Byte *data) {
	/* Nothing in the queue */
	
	if (!queue->count) return (NULL);
	/*
	Insert code here.
	Retrieve data from buffer, save it to "current" and "data"
	If the number of characters in the receive buffer is below
	certain level, then send XON.
	Increment front index and check for wraparound.
	*/
	do{
		if(queue->count > 0){
			(*data) = queue->data[queue->front];
			queue->count = queue->count - 1;
			if(queue->front < 7){
				queue->front = queue->front + 1;
			}
			else{
				queue->front = 0;
			}
		}
	}
	while((*data < 32) && (*data != LF) && (queue->count >0));
	
	if((queue->count < MAX_LOWERLIMIT) && (sent_xonxoff == XOFF)){
		sent_xonxoff = XON;
		send_xoff = false;
		send_xon = true;
		printf("Buffer < maximum lowerlimit\n");
		char test[2];
		test[0] = XON;
		ssize_t nBytesSnt = sendto(sockfd, test, sizeof(test), 4, (struct sockaddr*)&client, sizeof(client));
		printf("Mengirim XON\n");
		if(nBytesSnt < 0){
			printf("sendto failed\n");
		}
	}
}
