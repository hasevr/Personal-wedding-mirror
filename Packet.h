#ifndef PACKET_H
#define PACKET_H

enum Ports{ PORT_SEND = 11000 };
/*
struct PMediaLen{
	char packetId[4];
	int bufferSize;
	int length;
	PMediaLen():length(0), bufferSize(0){
		packetId[0] = 'l';
	}
};
*/
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
	char packetId[2];
	short count;
	unsigned char data[1024];
	PMediaData(){
		packetId[0] = 'd';
	}
};

#endif
