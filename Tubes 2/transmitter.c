/*
* File : transmitter.cpp
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "dcomm.h"
#include "SQ-ARQProtocol.h"

typedef struct ARQ {
	FRAME frame;
	int untilTimeout;
	Boolean ack;
} ARQ;

static Byte *recieved;
Boolean *shtdown;
ARQ listFrame[maxFrame];

// Error message
void error(const char *msg) {
    perror(msg);
    exit(0);
}

ARQ createARQ(FRAME frame) {
	
	ARQ ret;

	ret.frame = frame;
	ret.untilTimeout = 0;
	ret.ack = false;

	return ret;

}

// Binding to a socket
int initConnecton(int argc, char *argv[]) {

	int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    printf("Memulai socket untuk koneksi %s:%d ...\n",argv[1],portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    return sockfd;
}

//Data link layer for recieving ACK
ACKFormat recvACK(int sockfd, struct sockaddr_storage peer_addr, socklen_t peer_addr_len) {

    ACKFormat ack;
    Byte message[sizeof(ACKFormat)];

    int i = 0;
	do {

		char ch[1];
		peer_addr_len = sizeof(struct sockaddr_storage);
		recvfrom(sockfd,ch,1,0,(struct sockaddr *) &peer_addr, &peer_addr_len);
		*recieved = ch[0];
		printf("Terima ini %d\n",*recieved);

		if (*recieved == XOFF) {
			printf("XOFF diterima\n");
		} else if (*recieved == XON) {
			printf("XON diterima\n");
		} else {
			message[i] = *recieved;
			i++;
		}
	
	} while ((i<sizeof(ACKFormat)) && (!*shtdown));

	// Create stream of byte to struct
	memcpy(&ack,message,sizeof(message));

	return ack;
}

// Data Link Layer for sending a frame
void sendFrame(int sockfd, FRAME frame, struct sockaddr_storage peer_addr, socklen_t peer_addr_len) {

	// Variable declaration
	char red;
	int i = 0;
	Boolean isXOFF = false;

	// Create stream of byte from a frame
	Byte msg[sizeof(frame)];
	memcpy(msg, &frame, sizeof(frame));

	for (i=0;i<sizeof(frame);i++) {
		printf("%d ",msg[i]);
	}
	printf("\n");

	i=0;
	// Sending message byte per byte
	while (i < sizeof(frame)) {
		red = msg[i];

		if (*recieved == XOFF) {
			isXOFF = true;
		}
		if (*recieved == XON) {
			isXOFF = false;
		}
		while (isXOFF) {
			printf("Menunggu XON...\n");
			sleep(1);
			if (*recieved == XOFF) {
				isXOFF = true;
			}
			if (*recieved == XON) {
				isXOFF = false;
			}
		}

		printf("Mengirim byte ke-%d: '%c', %d\n",i+1,red,red);
		
		char buf[1];
		buf[0] = red;
		peer_addr_len = sizeof(struct sockaddr_storage);
		sendto(sockfd, buf, 1, 0, (struct sockaddr *) &peer_addr, peer_addr_len);

		i++;
		usleep(25 * 1000);

	}
	
	printf("Send finished\n");

}

// THE Sliding Window Protocol :D
void slidingProtocol(int sockfd, struct sockaddr_storage peer_addr, socklen_t peer_addr_len, ARQ * listOfFrame, int lastFrame) {
	// Start semding from frame 0
	int startFrame = 0;

	// Send until certain frame
	while (startFrame < lastFrame) {
		int i;
		Boolean move = true;
		int targetFrame = 0;
		// Sending all in the window
		for (i=0;i<startFrame+windowSize&&i<lastFrame;i++) {
			// If it is already ack-ed
			if (listOfFrame[i].ack) {
				// If there is no NAK behind
				if (move) {
					// Move the window
					targetFrame = i+1;
				}
			} else {
				move = false;
				targetFrame = i;
				// If timeout
				if (listOfFrame[i].untilTimeout == 0) {
					printf("Timeout!!! %d\n",i);
					// Reset timeout
					listOfFrame[i].untilTimeout = timeout;
					// Send frame
					sendFrame(sockfd, listOfFrame[i].frame, peer_addr, peer_addr_len);
				} else {
					printf("Wait!!! %d\n",i);
					// Decrease timeout
					listOfFrame[i].untilTimeout--;
				}
			}
		}

		// Move window for real
		startFrame = targetFrame;

		// Wait until the last frame is caught or timeout
		int backWindow = startFrame+windowSize-1;
//		while (!listOfFrame[backWindow].ack || (listOfFrame[backWindow].untilTimeout > 0)) {}
		printf("Ready for new cycle\n");

		usleep(1000 * 1000);

	}

	printf("Sliding window finished\n");

}

int main(int argc, char *argv[]) {

    /* code */

	// Global variables through multiple processes
	ARQ *ptr = mmap(0, maxFrame*sizeof(ARQ), PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS,  -1, 0);

	int j;
	for (j = 0; j < maxFrame; ++j)
	{
	    ptr[j] = listFrame[j];
	}

	recieved = mmap(NULL, sizeof *recieved, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*recieved = XON;

//	printf("Tadinya %d\n",*recieved);

	shtdown = mmap(NULL, sizeof *shtdown, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*shtdown = false;

	// Argument validation
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port filename\n", argv[0]);
       exit(0);
    }

    // Connection initialization
    int sockfd = initConnecton(argc, argv);

    // Beaucrachy for sending and receiving thorugh UDP
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;


    //Forking processes
	pid_t  pid;
	pid = fork();

	if (pid == 0)  {
		
		// Child process for receiving ack

		int j = 0;
		do {
			ACKFormat recv = recvACK(sockfd,peer_addr,peer_addr_len);

			if (testChecksumACK(recv)) {
				if (recv.ack == ACK) {
					printf("Pesan terkirim untuk frameno = %d\n",recv.frameno-1);
					ptr[recv.frameno-1].ack = true;
				} else {
					printf("Pesan tidak sampai\n");
					ptr[recv.frameno].untilTimeout = 0;
				}
			} else {
				printf("ACK corrupted\n");
			}
			j++;
		} while ((!*shtdown) && (j < 20));

		printf("Child finished\n");

	} else {

		// Parent process for sending message

		// Binding to a file
	    char filename[256];
	    strcpy(filename,argv[3]);
		FILE *f = fopen(filename,"rb");

    	char red;
    	int i = 0;
    	Byte message[MessageLength];

    	int indexFrame = 0;

    	// Framing message per MessageLength and create array of frame
		while (fscanf(f, "%c", &red) != EOF) {
			if (i < MessageLength) {
				message[i] = red;
				i++;
			} else {
//				sendFrame(sockfd,createFrame(1,message),peer_addr,peer_addr_len);
				ptr[indexFrame] = createARQ(createFrame(indexFrame,message));
				indexFrame++;
				i = 0;
			}
		}

		// Sending message shorter than MessageLength
		if (i != 0) {
			int j;
			for (j=i;j<MessageLength;j++) {
				message[j] = 0;
			}
//			sendFrame(sockfd,createFrame(1,message),peer_addr,peer_addr_len);
			ptr[indexFrame] = createARQ(createFrame(indexFrame,message));
			indexFrame++;
		}

		// Mengirim akhir file
		message[0] = Endfile;
		int j;
		for (j=1;j<MessageLength;j++) {
			message[j] = 0;
		}
//		sendFrame(sockfd,createFrame(1,message),peer_addr,peer_addr_len);
		ptr[indexFrame] = createARQ(createFrame(indexFrame,message));
		indexFrame++;

		fclose(f);

		slidingProtocol(sockfd,peer_addr,peer_addr_len,ptr,indexFrame);

		*shtdown = true;

		// Magic to end receiver
		char buf[1];
		buf[0] = 0;
		peer_addr_len = sizeof(struct sockaddr_storage);
		sendto(sockfd, buf, 1, 0, (struct sockaddr *) &peer_addr, peer_addr_len);
		
		wait(NULL);

		// Freeing global variables' memory
		munmap(recieved, sizeof *recieved);
		munmap(shtdown, sizeof *shtdown);
		munmap(ptr, sizeof *ptr);


		// Terminating connection
		printf("Mengakhiri koneksi\n");
		close(sockfd);

	}

	return 0;
}
	
