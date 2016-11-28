/*
* File : dcomm.h
*/
#include "dcomm.h"

#ifndef _SQ_ARQ_Protocol_H_
#define _SQ_ARQ_Protocol_H_

#define FrameLength 	16		/* Maximum length of message per frame */

int messageLength = 5 + FrameLength;

typedef struct FRAME {
	unsigned int soh;
	Byte frameno;
	unsigned int stx;
	Byte * data;
	unsigned int etx;
	Byte checksum;
} FRAME;

typedef struct ACKFormat {
	unsigned int ack;
	Byte frameno;
	Byte checksum;
} ACKFormat;

Byte getChecksumData(Byte frameno, Byte data[FrameLength]) {

	unsigned int sum = 0;

	sum += SOH;

	sum += frameno;

	sum += STX;

	int i;

	for (i=0;i<FrameLength;i++) {
		sum += data[i];
	}

	sum += ETX;

	return ~sum;

}


Byte getChecksumACK(Byte frameno, unsigned int ack) {

	unsigned int sum = 0;

	sum += frameno;
	
	sum += ack;

	return ~sum;

}


FRAME getFrame(Byte frameno, Byte data[FrameLength]) {
	
	FRAME ret;

	ret.soh = SOH;

	ret.frameno = frameno;

	ret.stx = STX;

	ret.data = data;

	ret.etx = ETX;

	ret.checksum = getChecksumData(frameno,data);

	return ret;

}

void createMsg(FRAME frame, Byte message[messageLength]) {

	message[0] = frame.soh;
	message[1] = frame.frameno;
	message[2] = frame.stx;

	int i;
	for (i=3;i<3+FrameLength;i++) {
		message[i] = frame.data[i-3];
	}

	message[3+FrameLength] = frame.etx;
	message[4+FrameLength] = frame.checksum;

}

ACKFormat getACK(Byte frameno, unsigned int ack) {
	ACKFormat af;

	af.ack = ack;
	af.frameno = frameno;
	af.checksum = getChecksumACK(frameno,ack);

	return af;
}

#endif