#ifndef PACKET_H
#define PACKET_H

#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>
#include "chunk.h"
//#include "hashlist.h"
//#include "peer.h"

#define MAX_HASHNUM_PER_PACK 50
#define BUFLEN2 1500
#define HEADER_LEN 16

typedef struct
{
	uint16_t Magic_Num;
	uint8_t Version_Num;
	uint8_t Packet_Type;
	uint16_t Hd_len;
	uint16_t Pack_len;
	uint32_t Seq_Num;
	uint32_t ACK_Num;
	uint8_t Chunkhash_Num; 
	char *payload;
/*****only useful for data packets*********/	
	time_t timestamp;
	int ACKed_num;
}packet;

typedef struct {
	char buf[BUFLEN2];
	int buf_len;
}buf;

void print_pack(packet *pack);
int binary2packet(char *stream,packet* pack);
char * serial_pack(packet *pack);
void del_packet(packet *pack);



/****get the first packet out and fill the packet structure******/
void get_firstpack(buf *buffer,packet* pack);
/*****retrieve the first packet-stream from buffer******/
char* get_firststream(buf *buffer);

void form_DATA_pack(packet* datapack,char* data,int data_len,int32_t seqno);
void form_ACK_pack(packet* pack,int32_t seqno);
#endif
