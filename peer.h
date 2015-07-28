#ifndef PEER_H
#define PEER_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "packet.h"
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "hashlist.h"
#include "sock_io.h"
#include "win_log.h"

typedef struct{
	int id;
	struct sockaddr_in addr;
	hash_chunk * haslist;

	Download_conn mydownload;
	Upload_conn myupload;
}peer_me;


bt_config_t config;
peer_me me;
int sock;
void init_peerme(peer_me *me,bt_config_t *config); //init the structure peer_me, which contains the whole scope of current prre
void peer_run(bt_config_t *config);
/*****every receive GET command, reset my download structure********/
void reset_mydownload(peer_me* me,char* chunkfile, char* outputfile);

void process_inbound_udp(int sock,buf *buf);

void handle_user_input(char *line, void *cbdata);
void process_get(char *chunkfile, char *outputfile); //after user type in GET command, process request

void print_peer_me(peer_me* me); //printf all peer_me elements, for debug

/***send I hav pack when I receive WhoHas pack, if I dont have the chunk,send nothing back******/
void IHav_sender(packet* WhoHas_pack,hash_chunk* haslist,struct sockaddr_in *toaddr);
/***iteratively send whohas packets with every 50 hashes in its payload***/
void WhoHas_sender(hash_chunk *needlist,bt_peer_t *peers);

int GET_sender(Download_conn* down);
/*****move qualified nodes from get_tosend_list  to  get_sent_list,and send it*******/
int pick_tosend_GET(Download_conn* down,int maxpick_num);

void dealwithincomepack(packet *pack,struct sockaddr_in *toaddr);
/*****after receive IHav pack, decise which hash need to add to get_tosend_list*****/
void fill_toget_list(packet *ihav_pack,hash_chunk** get_tosend,struct sockaddr_in *from_addr);

/***check the get_sent_list to see any GET request time out, if timeout for more than 3 times, drop the GET request***/
/***return the number of the tomeout chunks***/
int check_timeout(hash_chunk** list);
/***if the receiver doesnot receive data for long time, quit the download***/
int check_recvdata_timeout(hash_chunk* curr);
/****check every upload timeout, if 16 sec no any ACK come,drop this connection*****/
int check_recvack_timeout(hash_chunk* curr);

int init_upload(packet* getpack,struct sockaddr_in* dst);
void abort_upload(hash_chunk* todel);

void init_download(hash_chunk* down_conn);
void finish_one_download(hash_chunk* todel);
void complete_mydownload(Download_conn* down);
#endif
