/*
* File : transmitter.cpp
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include "dcomm.h"

int main(int argc, char const *argv[])
{
	/* code */
	int sockfd, newsockfd;	// Socket File Descriptor
    char* portnostr = argv[2];
    char* textfile;  
    int portno;
    int clilen;
     
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr; // Socket Addresses
	int n;
     
	char hostname[256];
	bzero(hostname,256);
	gethostname(hostname, 255);

    // Try opening socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		error("setsockopt(SO_REUSEADDR) failed");
     
    // Reset buffer
    bzero((char *) &serv_addr, sizeof(serv_addr));
     
    // Bind the server address to the server socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr)) < 0) error("ERROR on binding");
     
     // Set listen paramater         
    listen(sockfd,5);
     
    printf("Listening for connection\n");
     
    char buf[256];
    while(1) {

	}

	printf("Server Down\n");
	close(sockfd);
	return 0;
}
	