#include "packet.h"

void print_pack(packet *pack)
{
	char hex[2000]={0};
	binary2hex((uint8_t*) pack->payload, pack->Chunkhash_Num*20, hex);
	printf("Magic num: %d\nVersion: %d\nPacket_Type: %d\nHd lenth: %d\nPack len: %d\nSeq num: %d\nACK num: %d\nhash_num: %d\nACKed %d Times\nTimestamp:%d\n",
		    pack->Magic_Num,pack->Version_Num,pack->Packet_Type,pack->Hd_len,pack->Pack_len,pack->Seq_Num,pack->ACK_Num,pack->Chunkhash_Num,pack->ACKed_num,(int)pack->timestamp);
}


int binary2packet(char *stream,packet* pack)
{
	int non_payload_len=0;
	memcpy((char*)pack,stream,HEADER_LEN);
	pack->Magic_Num=ntohs(pack->Magic_Num);
	pack->Hd_len=ntohs(pack->Hd_len);
	pack->Pack_len=ntohs(pack->Pack_len);
	pack->Seq_Num=ntohl(pack->Seq_Num);
	pack->ACK_Num=ntohl(pack->ACK_Num);

	non_payload_len=16;

	if(pack->Packet_Type<=1)
	{
		strncpy((char*)(&(pack->Chunkhash_Num)),stream+HEADER_LEN,1);
		non_payload_len=20;
	}

	if(pack->Packet_Type<4 && pack->Pack_len - non_payload_len>0)
	{
		pack->payload=(char*)malloc(pack->Pack_len - non_payload_len+1);
		memcpy(pack->payload,stream+non_payload_len,pack->Pack_len - non_payload_len);
	}

	return 1;
}

char* serial_pack(packet *pack)
{
	int non_payload_len=0;
	char *stream=(char*)malloc(pack->Pack_len+1);
	bzero(stream,pack->Pack_len+1);

	pack->Magic_Num=htons(pack->Magic_Num);
	pack->Hd_len=htons(pack->Hd_len);
	pack->Pack_len=htons(pack->Pack_len);
	pack->Seq_Num=htonl(pack->Seq_Num);
	pack->ACK_Num=htonl(pack->ACK_Num);
	
	memcpy(stream,(char*)(&(pack->Magic_Num)),HEADER_LEN);

	pack->Magic_Num=ntohs(pack->Magic_Num);
	pack->Hd_len=ntohs(pack->Hd_len);
	pack->Pack_len=ntohs(pack->Pack_len);
	pack->Seq_Num=ntohl(pack->Seq_Num);
	pack->ACK_Num=ntohl(pack->ACK_Num);

	non_payload_len=HEADER_LEN;
	if (pack->Packet_Type<=1)
	{
		*((uint8_t*)(stream+HEADER_LEN))=pack->Chunkhash_Num;
		non_payload_len=20;
	}

	memcpy(stream+non_payload_len,pack->payload,pack->Pack_len - non_payload_len);
	return stream;  //stream len is one bit longer than packet length
}

void del_packet(packet *pack)
{
	if (pack->payload)
	{
		free(pack->payload);
	}
	
	free(pack);
}


/****get the first packet out and fill the packet structure******/
void get_firstpack(buf *buffer,packet* pack)
{
  char * newcome_stream=get_firststream(buffer);
  if(newcome_stream!=NULL){
      binary2packet(newcome_stream, pack);
      free(newcome_stream); 
  }
} 

/*****retrieve the first packet-stream from buffer******/
char* get_firststream(buf *buffer)
{
  char * new_stream;
  uint16_t stream_len=0;
  if (buffer->buf_len<HEADER_LEN)
  {
    return NULL;
  }
  memcpy((char*)(&stream_len),buffer->buf+6,2);
  stream_len=ntohs(stream_len);
 // printf("come pack length:%d\n", stream_len);

  new_stream=(char *)malloc(stream_len+1);
  bzero(new_stream,stream_len+1);

  memcpy(new_stream,buffer->buf,stream_len);
  memcpy(buffer->buf,buffer->buf+stream_len,BUFLEN2 - stream_len);
  memset(buffer->buf + BUFLEN2 - stream_len,0,stream_len);
  buffer->buf_len-=stream_len;
  return new_stream;
}

void form_DATA_pack(packet* datapack,char* data,int data_len,int32_t seqno)
{
	bzero(datapack,sizeof(packet));

	datapack->Packet_Type=3;
	datapack->Hd_len=HEADER_LEN;
	
	datapack->Magic_Num=15441;
	datapack->Version_Num=1;	
	//datapack->ACK_Num=0;
	datapack->Seq_Num=seqno;
	datapack->timestamp=0; 
	//datapack->ACKed_num=0;
	datapack->Pack_len=HEADER_LEN+data_len;

	datapack->payload=(char*)malloc(data_len+1);
	bzero(datapack->payload,data_len+1);

	memcpy(datapack->payload,data,data_len);
}

void form_ACK_pack(packet* pack,int32_t seqno)
{
	bzero(pack,sizeof(packet));
	pack->Packet_Type=4;
	pack->Hd_len=HEADER_LEN;
	
	pack->Magic_Num=15441;
	pack->Version_Num=1;
	pack->Pack_len=HEADER_LEN;
	pack->ACK_Num=seqno;
}