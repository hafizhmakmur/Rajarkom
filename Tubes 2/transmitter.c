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


void error(const char *msg) {
    perror(msg);
    exit(0);
}

int recvACK(int sockfd, char * msg, int length) {
	do {

		char ch[1];
		peer_addr_len = sizeof(struct sockaddr_storage);
		recvfrom(sockfd,ch,1,0,(struct sockaddr *) &peer_addr, &peer_addr_len);
		*recieved = ch[0];

		if (*recieved == XOFF) {
			printf("XOFF diterima\n");
		} else if (*recieved == XON) {
			printf("XON diterima\n");
		}
	
	} while (!*shtdown);

	return 0;
}

int sendFrame(int sockfd, char * msg, int length) {


	return 0;
}



int main(int argc, char *argv[]) {

	int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256], filename[256];

    /* code */

	recieved = mmap(NULL, sizeof *recieved, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*recieved = XON;


	shtdown = mmap(NULL, sizeof *shtdown, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	*shtdown = false;

    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port filename\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    strcpy(filename,argv[3]);

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
    

    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;

	pid_t  pid;
	pid = fork();
	
	if (pid == 0)  {
		

		printf("Child finished\n");

	} else {;
    	char red;
    	int i = 0;

		while (i < length) {
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

		*shtdown = true;
		
		printf("Send finished\n");

		wait(NULL);

	}

	munmap(recieved, sizeof *recieved);
	munmap(shtdown, sizeof *shtdown);
	FILE *f = fopen(filename,"rb");

	int i = 1;
	
	char * line = NULL;
    size_t len = 0;
    ssize_t read;

	while ((read = getline(&line, &len, f)) != -1) {

        printf("Mengirim string ke-%d dengan panjang %d: '%s'\n",i,strlen(line),line);

		int sent = sendFrame(sockfd, line, strlen(line));

		printf("Kiriman selesai\n");

		i++;

		usleep(25 * 1000);
    }

	fclose(f);

	printf("Mengakhiri koneksi\n");
	close(sockfd);
	
	
	return 0;
}
	