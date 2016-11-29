/*
* File : dcomm.h
*/
#include "dcomm.h"

#ifndef _SQ_ARQ_Protocol_H_
#define _SQ_ARQ_Protocol_H_

#define MessageLength 	16		/* Maximum length of message per frame */

int frameLength = 5 + MessageLength;

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

Byte getChecksumData(Byte frameno, Byte data[MessageLength]) {

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

	return (sum+frame.checksum) == 0;

}


Byte getChecksumACK(Byte frameno, unsigned int ack) {

	unsigned int sum = 0;

	sum += frameno;
	
	sum += ack;

	return ~sum;

}

Boolean testChecksumACK(ACKFormat ackf) {

	unsigned int sum = 0;

	sum += ackf.frameno;
	
	sum += ackf.ack;

	return (sum+ackf.checksum) == 0;

}


FRAME createFrame(Byte frameno, Byte data[MessageLength]) {
	
	FRAME ret;

	ret.soh = SOH;

	ret.frameno = frameno;

	ret.stx = STX;

	ret.data = data;

	ret.etx = ETX;

	ret.checksum = getChecksumData(frameno,data);

	return ret;

}

/*
void createMsgFrame(FRAME frame, Byte message[frameLength]) {

	message[0] = frame.soh;
	message[1] = frame.frameno;
	message[2] = frame.stx;

	int i;
	for (i=3;i<3+MessageLength;i++) {
		message[i] = frame.data[i-3];
	}

	message[3+frameLength] = frame.etx;
	message[4+frameLength] = frame.checksum;

}
*/

/*
FRAME getFrame(Byte message[frameLength]) {

	FRAME ret;

	if (message[0] != SOH) return NULL;
	ret.soh = SOH;
}
*/

ACKFormat createACK(Byte frameno, unsigned int ack) {
	ACKFormat af;

	af.ack = ack;
	af.frameno = frameno;
	af.checksum = getChecksumACK(frameno,ack);

	return af;
}

#endif