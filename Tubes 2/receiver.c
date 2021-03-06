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
#include "SQ-ARQProtocol.h"

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
int FrameNo = 0;
struct sockaddr_in client, local;
typedef struct CaFrame{
	Byte data[sizeof(FRAME)];
	int LastIdx;
} CaFrame;

CaFrame Seed;
FRAME f;
FRAME FrameList[maxFrame];
Boolean *Finish = false;

/*define Limits*/
#define MIN_UPPERLIMIT 4
#define MAX_LOWERLIMIT 1


void error(const char *msg) {
    perror(msg);
    exit(0);
}

Boolean IsLastFrame(FRAME fr){
	Boolean LastFrame = false;
	for(int i = 0; i<MessageLength; i++){
		if(fr.data[i] == Endfile){
			LastFrame = true;
		}
	}
	return LastFrame;
}


void SendACK (int sockfd, Byte ack, Byte Frameno){
	ACKFormat ackmsg;
	ackmsg.ack = ack;
	ackmsg.frameno = Frameno;
	ackmsg.checksum = getChecksumACK(Frameno, ack);
	Byte buf[sizeof(ackmsg)];
	memcpy(buf, &ackmsg, sizeof(ackmsg));
	int i = 0;
	while (i < sizeof(ackmsg)){
		char test[1];
		test[0] = buf[i];
		ssize_t nACKSnt = sendto(sockfd, test, sizeof(test), 0, (struct sockaddr*)&client, sizeof(client));
		if(nACKSnt <0){
			printf("ACK sendto failed\n");
		}
		i++;
	}

	if(ack == ACK){
		printf("Mengirim ACK Frame No. %d\n", Frameno);
	}
	else{
		printf("Mengirim NAK Frame No. %d\n", Frameno);
	}
	
}

int main(int argc, char const *argv[])
{
	/* code */

	Byte *ptr = mmap(0, RXQSIZE, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS,  -1, 0);

	int j;
	for (j = 0; j < RXQSIZE; ++j)
	{
	    ptr[j] = rxbuf[j];
	}

	rxq = mmap(NULL, sizeof *rxq, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	rxq->count = 0;
	rxq->front = 0;
	rxq->rear = 0;
	rxq->maxsize = RXQSIZE;
	rxq->data = ptr;

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

	Finish = mmap(NULL, sizeof *Finish, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*Finish = false;



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

	// Wait until client connect
	int clilen = sizeof(client);
	int newsockfd = accept(sockfd, (struct sockaddr*)&client, &clilen);

	if (newsockfd < 0) error("ERROR on accept");
	
	Seed.LastIdx = -1;
	pid_t  pid = fork();
	if(pid != 0){
		/*** IF PARENT PROCESS ***/
		while ( true ) {

			char * s = rcvchar(newsockfd, rxq);
			
			/* Quit on end of file */	
			if (*Finish) {
				wait(NULL);
				printf("Mengakhiri program\n");
				munmap(ptr, sizeof *ptr);
				munmap(rxq, sizeof *rxq);
				munmap(send_xon, sizeof *send_xon);
				munmap(send_xoff, sizeof *send_xoff);
				munmap(sent_xonxoff, sizeof *sent_xonxoff);
				munmap(rcvdByte, sizeof *rcvdByte);
				munmap(cnsmByte, sizeof *cnsmByte);
				exit(0);
			}
	
		}
		munmap(rxq, sizeof *rxq);
	} else {
	
		/*** ELSE IF CHILD PROCESS ***/
		while ( true ) {
			/* Call q_get */
			Byte *coba = q_get(newsockfd, rxq, &c);
			if(coba != NULL){
				if(rxq->front > 0){
				//	if((rxq->data[rxq->front - 1] != Endfile) && (rxq->data[rxq->front - 1] != CR) && (rxq->data[rxq->front - 1] != LF)){
						printf("Mengkonsumsi byte ke-%d: '%c', %d\n", ++*cnsmByte, rxq->data[rxq->front - 1], rxq->data[rxq->front - 1]);
						Seed.data[Seed.LastIdx + 1] = rxq->data[rxq->front -1];
						Seed.LastIdx++;
				/*	}
					else if(rxq->data[rxq->front - 1] == Endfile){
						//do nothing
						exit(0);
					}
				*/
				}
				else{
				//	if((rxq->data[RXQSIZE] != Endfile) && (rxq->data[RXQSIZE] != CR) && (rxq->data[RXQSIZE] != LF)){
						printf("Mengkonsumsi byte ke-%d: '%c', %d\n", ++*cnsmByte, rxq->data[RXQSIZE], rxq->data[RXQSIZE]);
						Seed.data[Seed.LastIdx + 1] = rxq->data[RXQSIZE];
						Seed.LastIdx++;
				/*	}
					else if(rxq->data[RXQSIZE] == Endfile){
						exit(0);
					}
				*/
				}
				if(Seed.LastIdx == (sizeof(FRAME)-1)){
					memcpy(&f,Seed.data,sizeof(Seed.data));
					if ((f.soh == SOH) && (f.stx == STX) && (f.etx == ETX)) {
					/*
					if((Seed.data[0] == SOH) && (Seed.data[2] == STX) && (Seed.data[MessageLength+3] == ETX)){
						f.soh = Seed.data[0];
						f.frameno = Seed.data[1];
						f.stx = Seed.data[2];
						int idx = 0;
						while (idx < MessageLength){
							f.data[idx] = Seed.data[idx+3];
							idx++;
						}
						f.etx = Seed.data[MessageLength+3];
					*/
						int random = rand()%3;
						printf("Hasil random : %d\n", random);
						if(testChecksumData(f) && random){
							SendACK(newsockfd, ACK, f.frameno+1);
							printf("Frame Nomor %d: ", f.frameno);
							for(int j = 0; j < MessageLength; j++){
								if(f.data[j] != NULL){
									printf("%c", f.data[j]);
								}
							}
							printf("\n");
							if(IsLastFrame(f)){
								printf("Finished\n");
								*Finish = true;
								exit(0);
							}
						}
						else{
							SendACK(newsockfd, NAK, f.frameno+1);
						}
					}
					else{
						SendACK(newsockfd, NAK, Seed.data[1]);
					}
					Seed.LastIdx = -1;
				}
				
			}
			/* Can introduce some delay here. */
			usleep(DELAY * 2000);
		}
	}
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
	
	while(*send_xoff){}

	int clilen = sizeof(client);
	ssize_t nBytesRcvd = recvfrom(sockfd, t, sizeof(t), 0, (struct sockaddr*)&client, &clilen);
	if(nBytesRcvd < 0){
		printf("recvfrom failed\n");
	}
	else{

	//	if((t[0] != Endfile) && 
	//	if (t[0] > 32 || t[0] == CR || t[0] == LF || t[0] == Endfile) {
			
			printf("Menerima byte ke-%d\n", *rcvdByte);
				
			queue->data[queue->rear] = t[0];
			queue->count = queue->count + 1;
			
			if(queue->rear < RXQSIZE){
				queue->rear = queue->rear + 1;
			}
			else{
				queue->rear = 0;
			}
			*rcvdByte = *rcvdByte + 1;
			
	//	} else {
	//		printf("Menerima illegal character\n");
	//	}
/*
		}
		else if(t[0] <= 32){
			printf("Menerima illegal character\n");
		}
*/
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
			printf("XOFF sendto failed\n");
		}
	}

	return &t[0];
 
}

	/* q_get returns a pointer to the buffer where data is read or NULL if
	* buffer is empty.3wax
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

	Boolean validchar = false;
	do{
		if(queue->count > 0){
			(*data) = queue->data[queue->front];
			queue->count = queue->count - 1;
			if(queue->front < RXQSIZE){
				queue->front = queue->front + 1;
			}
			else{
				queue->front = 0;
			}
			
//			if (*data > 32 || *data == CR || *data == LF || *data == Endfile) {
				validchar = true;
//			}
		}
	} while (!validchar);

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
			printf("XON sendto failed\n");
		}
	}

	return data;
}

