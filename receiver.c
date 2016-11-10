/*
* File : receiver.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
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
QTYPE *rxq;
Byte *sent_xonxoff;
Boolean *send_xon = false, *send_xoff = false;

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(int sockfd, QTYPE *, Byte *);

/* Other variables */
Byte t[2];
int *rcvdByte = 0;
int *cnsmByte = 0;
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

	rxq = mmap(NULL, sizeof *rxq, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	rxq->count = 0;
	rxq->front = 0;
	rxq->rear = 0;
	rxq->maxsize = RXQSIZE;
	rxq->data = rxbuf;

	send_xon = mmap(NULL, sizeof *send_xon, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*send_xon = false;

	send_xoff = mmap(NULL, sizeof *send_xoff, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*send_xoff = false;

	sent_xonxoff = mmap(NULL, sizeof *sent_xonxoff, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*sent_xonxoff = XON;

	rcvdByte = mmap(NULL, sizeof *rcvdByte, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*rcvdByte = 0;

	cnsmByte = mmap(NULL, sizeof *cnsmByte, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*cnsmByte = 0;


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

	// Wait until client connect
	int clilen = sizeof(client);
	int newsockfd = accept(sockfd, (struct sockaddr*)&client, &clilen);

	if (newsockfd < 0) error("ERROR on accept");

	printf("Here I am\n");
	pid_t  pid = fork();
	if(pid != 0){
		printf("Parent here %d\n",pid);
		/*** IF PARENT PROCESS ***/
		int i;
		while ( i < 20 ) {
			char * s = rcvchar(newsockfd, rxq);
			if (s != NULL) c = *s;

			printf("Back from the other side\n");
	
			/* Quit on end of file */	
			if (c == Endfile) {
				exit(0);
			}
			i++;
		}

		munmap(rxq, sizeof *rxq);
	} else {
	
		/*** ELSE IF CHILD PROCESS ***/
		printf("Child reporting\n");
		int i=0;
		while ( i < 20 ) {
			/* Call q_get */
			Byte *coba = q_get(newsockfd, rxq, &c);
			printf("Alola %s\n",coba);
			if(coba != NULL){
				if(rxq->front > 0){
					if((rxq->data[rxq->front - 1] != Endfile) && (rxq->data[rxq->front - 1] != CR) && (rxq->data[rxq->front - 1] != LF)){
						printf("Mengkonsumsi byte ke-%d: '%c'\n", ++*cnsmByte, rxq->data[rxq->front - 1]);
					}
					else if(rxq->data[rxq->front - 1] == Endfile){
						//do nothing
						exit(0);
					}
				}
				else{
					if((rxq->data[7] != Endfile) && (rxq->data[7] != CR) && (rxq->data[7] != LF)){
						printf("Mengkonsumsi byte ke-%d: '%c'\n", ++*cnsmByte, rxq->data[7]);
					}
					else if(rxq->data[7] == Endfile){
						exit(0);
					}
				}
				
			}
			
			/* Can introduce some delay here. */
			usleep(DELAY * 1000);
			i++;
		}


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
	

	while(*send_xoff){
//		printf("Consume already %d!\n",queue->count);
	}

//	printf("Greetings\n");
	int clilen = sizeof(client);
	ssize_t nBytesRcvd = recvfrom(sockfd, t, sizeof(t), 0, (struct sockaddr*)&client, &clilen);
//	printf("How's there\n");
	if(nBytesRcvd < 0){
		printf("recvfrom failed\n");
	}
	else{
		queue->data[queue->rear] = t[0];
		printf("Must be here %d\n", queue->data[queue->rear]);
		queue->count = queue->count + 1;
		if(queue->rear < 7){
			queue->rear = queue->rear + 1;
		}
		else{
			queue->rear = 0;
		}
		*rcvdByte = *rcvdByte + 1;
		printf("Pass this %d\n",queue->count);
	}
	if((t[0] != Endfile) && (t[0] != CR) && (t[0] != LF)){
		printf("Menerima byte ke-%d\n", *rcvdByte);
	}
	
	if((queue->count > MIN_UPPERLIMIT) && (*sent_xonxoff == XON)){
		*sent_xonxoff = XOFF;
		*send_xoff = true;
		*send_xon = false;
		printf("Buffer > minimum upperlimit\n");
		char test[1];
		test[0] = XOFF;
		ssize_t nBytesSnt = sendto(sockfd, test, sizeof(test), 4, (struct sockaddr*)&client, sizeof(client));
		printf("Mengirim XOFF\n");
		if(nBytesSnt < 0){
			printf("sendto failed\n");
		}
	}
//	printf("???\n");
	return &t[0];
 
/*
	else {
		Byte *buff = 0;
		printf("!!! %s\n",buff);
		return buff;
	}
*/
}

	/* q_get returns a pointer to the buffer where data is read or NULL if
	* buffer is empty.
	*/

static Byte *q_get(int sockfd, QTYPE *queue, Byte *data) {
	/* Nothing in the queue */
	
	printf("Now i am done %d\n",queue->count);

	if (!queue->count) return (NULL);
	/*
	Insert code here.
	Retrieve data from buffer, save it to "current" and "data"
	If the number of characters in the receive buffer is below
	certain level, then send XON.
	Increment front index and check for wraparound.
	*/
	printf("When I have evolved\n");
	Boolean validchar = false;
	int i = 0;
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
			
			printf("Laporan %d xx%cxx\n",queue->count,*data);
			if (*data > 32 || *data == CR || *data == LF || *data == Endfile) {
				validchar = true;
			}
		}
		i++;
	}
//	while((*data < 32) && (*data != LF) && (queue->count >0));
	while (!validchar && i < 20);
	
	printf("Akhiri %d %d\n",*sent_xonxoff,XOFF);

	if((queue->count < MAX_LOWERLIMIT) && (*sent_xonxoff == XOFF)){
		*sent_xonxoff = XON;
		*send_xoff = false;
		*send_xon = true;
		printf("Buffer < maximum lowerlimit\n");
		char test[1];
		test[0] = XON;
		ssize_t nBytesSnt = sendto(sockfd, test, sizeof(test), 4, (struct sockaddr*)&client, sizeof(client));
		printf("Mengirim XON\n");
		if(nBytesSnt < 0){
			printf("sendto failed\n");
		}
	}
}
