/*
* File : dcomm.h
*/

/* Referensi untuk Sliding Protocol 

Checksum : http://scanftree.com/programs/c/implementation-of-checksum/


*/

#include "dcomm.h"

#ifndef _SQ_ARQ_Protocol_H_
#define _SQ_ARQ_Protocol_H_

#define MessageLength 	16		/* Maximum length of message per frame */

#define maxFrame 	500
#define windowSize 	5
#define timeout 	5

typedef struct FRAME {
	Byte soh;
	Byte frameno;
	Byte stx;
	Byte data[MessageLength];
	Byte etx;
	int checksum;
} FRAME;

typedef struct ACKFormat {
	Byte ack;
	Byte frameno;
	int checksum;
} ACKFormat;

int getChecksumData(Byte frameno, Byte data[MessageLength]) {

	unsigned int sum = 0;

	sum += SOH;

	sum += frameno;

	sum += STX;

	int i;

	for (i=0;i<MessageLength;i++) {
		sum += data[i];
	}

	sum += ETX;

	return ~sum;

}

Boolean testChecksumData(FRAME frame) {

	unsigned int sum = 0;

	sum += frame.soh;

	sum += frame.frameno;

	sum += frame.stx;

	int i;

	for (i=0;i<MessageLength;i++) {
		sum += frame.data[i];
	}

	sum += frame.etx;

	return ~(sum+frame.checksum) == 0;

}


int getChecksumACK(Byte frameno, unsigned int ack) {

	unsigned int sum = 0;

	sum += frameno;
	
	sum += ack;

	return ~sum;

}

Boolean testChecksumACK(ACKFormat ackf) {

	unsigned int sum = 0;

	sum += ackf.frameno;
	
	sum += ackf.ack;

	return ~(sum+ackf.checksum) == 0;

}


FRAME createFrame(Byte frameno, Byte data[MessageLength]) {
	
	FRAME ret;

	ret.soh = SOH;

	ret.frameno = frameno;

	ret.stx = STX;

	int i;
	printf("Terima ");
	for (i=0;i<MessageLength;i++) {
		printf("%d ",data[i]);
	}
	printf("\n");

	memcpy(&ret.data,data,MessageLength);

	ret.etx = ETX;

	ret.checksum = getChecksumData(frameno,data);

	return ret;

}

ACKFormat createACK(Byte frameno, unsigned int ack) {
	ACKFormat af;

	af.ack = ack;
	af.frameno = frameno;
	af.checksum = getChecksumACK(frameno,ack);

	return af;
}

#endif
