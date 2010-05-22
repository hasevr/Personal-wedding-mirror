#ifndef PACKET_H
#define PACKET_H

enum Ports{ PORT_SEND = 11000 };
struct PMediaTypeAndLen{
	char packetId[4];
	int bufferSize;
	int length;
	AM_MEDIA_TYPE mt;
	unsigned char format[1000 - sizeof(AM_MEDIA_TYPE)];
	PMediaTypeAndLen(){
		packetId[0]='t';
	}
};

struct PMediaData{
	enum DataLen{DATALEN=1024 + 256};
	char packetId[2];
	short count;
	unsigned char data[DATALEN];
	PMediaData(){
		packetId[0] = 'd';
	}
};

#endif
