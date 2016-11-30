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

static Byte *recieved;
Boolean *shtdown;

// Error message
void error(const char *msg) {
    perror(msg);
    exit(0);
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
    Byte message[3];

    int i = 0;
	do {

		char ch[1];
		peer_addr_len = sizeof(struct sockaddr_storage);
		recvfrom(sockfd,ch,1,0,(struct sockaddr *) &peer_addr, &peer_addr_len);
		*recieved = ch[0];

		if (*recieved == XOFF) {
			printf("XOFF diterima\n");
		} else if (*recieved == XON) {
			printf("XON diterima\n");
		} else {
			message[i] = *recieved;
			i++;
		}
	
	} while ((i<3) && (!*shtdown));

	// Create stream of byte to struct
	memcpy(&ack,message,sizeof(message));

	return ack;
}

// Data Link Layer for sending a frame
void sendFrame(int sockfd, FRAME frame, struct sockaddr_storage peer_addr, socklen_t peer_addr_len) {

	// Variable declaration
	char red;
	int i = 0;

	// Create stream of byte from a frame
	Byte msg[sizeof(frame)];
	memcpy(msg, &frame, sizeof(frame));

	for (i=0;i<sizeof(frame);i++) {
		printf("%d ",msg[i]);
	}

	i=0;
	// Sending message byte per byte
	while (i < MessageLength) {
		red = msg[i];

		while (*recieved != XON) {
			printf("Menunggu XON...\n");
			sleep(1);
		}

		printf("Mengirim byte ke-%d: %c,'%d'\n",i+1,red,red);
		
		char buf[1];
		buf[0] = red;
		peer_addr_len = sizeof(struct sockaddr_storage);
		sendto(sockfd, buf, 1, 0, (struct sockaddr *) &peer_addr, peer_addr_len);

		i++;
		usleep(25 * 1000);

	}
	
	printf("Send finished\n");

}



int main(int argc, char *argv[]) {

    /* code */

	// Global variables through multiple processes
	recieved = mmap(NULL, sizeof *recieved, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*recieved = XON;


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
					printf("Pesan terkirim\n");
				} else {
					printf("Pesan tidak sampai\n");
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

    	// Framing message per MessageLength and sending it
		while (fscanf(f, "%c", &red) != EOF) {
			if (i < MessageLength) {
				message[i] = red;
				i++;
			} else {
				int k;
				printf("Kirimkan ");
				for (k=0;k<MessageLength;k++) {
					printf("%d ",message[k]);
				}
				printf("\n");
				sendFrame(sockfd,createFrame(1,message),peer_addr,peer_addr_len);
				i = 0;
			}
		}

		// Sending message shorter than MessageLength
		if (i != 0) {
			int j;
			for (j=i;j<MessageLength;j++) {
				message[j] = 0;
			}
			sendFrame(sockfd,createFrame(1,message),peer_addr,peer_addr_len);
		}

		// Mengirim akhir file
		message[0] = Endfile;
		int j;
		for (j=1;j<MessageLength;j++) {
			message[j] = 0;
		}
		sendFrame(sockfd,createFrame(1,message),peer_addr,peer_addr_len);

		fclose(f);
		*shtdown = true;
		
		wait(NULL);

		// Freeing global variables' memory
		munmap(recieved, sizeof *recieved);
		munmap(shtdown, sizeof *shtdown);


		// Terminating connection
		printf("Mengakhiri koneksi\n");
		close(sockfd);

	}

	return 0;
}
	