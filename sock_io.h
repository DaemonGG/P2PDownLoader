#ifndef SOCK_IO_H
#define SOCK_IO_H

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <error.h>
#include <stdio.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "spiffy.h"
//#include "hashlist.h"
#include "packet.h"
#include "bt_parse.h"

//#include "peer.h"

#define SENDER_BUF_SIZE 514 //the size of sender buffer is 512 packets
#define MAX_PACKET_SIZE 1040// max of size per packet is 1040 bytes(1KB of data + 16B header)
#define TCP_MAXWIN 128//???
#define INITIAL_CONGESTION_THRESHOLD 64
#define CHUNK_SIZE 524288
#define TIMEOUT_SEC 2
typedef int32_t Seqno;

struct _Window
{
	char hash[20];
	struct sockaddr_in dst;
	uint32_t flowid;
	int ssthresh;

	Seqno LPA;//LAST PACKET ACKed 
	Seqno LPS;//LAST PACKET SENT
	Seqno LPR;//LAST PACKET READY TO put into packet
	int count; // means how many packets can send as not exceeds the window size
	float congwin_size; 
	packet* sender_buf[SENDER_BUF_SIZE];
	struct _Window *next;

};
typedef struct _Window Window;

typedef struct 
{
	char hash[20];
	packet* recv_buf[SENDER_BUF_SIZE];
	Seqno NPE; //next packet expected
	//Seqno LPR; //last pack read
	int recv_bytesnum;
}Recv_State;

typedef struct 
{
	Window* whd;
	int window_num;
}W_HD;
/**send the packet to all peers in the 'peers' list***/
int broadcast(int sock,packet* pack, bt_peer_t *peers,int myid);
/******having the packet struct and dest addr,send one packet*******/
int send_pack(int mysock,packet* pack,struct sockaddr_in* toaddr);
/****send the len of data into the sender_buf, fragment data into 1024 each forming a 1040 packet*****/
int p2p_sender(char* data,Window *SW,int len);
/****read chunk from file and send one chunk into sender_buf, means 512K of data*******/
int send_onechunk(Window *SW,int offset,char *where);
/*****check the data pack timeout of each recv buffer******/
int check_datapack_timeout(Window* sw);
/******according to congestion window, send packets that are allowed,return how many were sent******/
int window_sender(int mysock,Window* SW);
/*****return current congestion win size*****/
int upon_receive_ACK(packet* ack_pack,Window* curr_win);
void upon_receiving_datapack(int mysock,packet* pack, Recv_State* rev,struct sockaddr_in* addr);
/******move the read packets' data into the outputfile after hash check*********/
/***return howmany bytes are reap*****/
int reap(Recv_State* recv,int offset,int outfd);
/*******check the hash value of the whole chunk. compare with the GET *.chunks file item**********/
int check_onechunk_hash(char *data,char* pattern);

/***create sender window structure and how to free it****/
Window* create_SW(char *hash,struct sockaddr_in* dst);
void release_window(Window **pw);
/***create receiver state structure and how to free it****/
Recv_State* create_Recver(char* hash);
void del_Recver(Recv_State** R);

/****these funciton is only used for debug****/
void check(int chunkid, Recv_State* recv);
void printWindow(Window* curr);
void printRecvState(Recv_State* recv);
#endif
