#ifndef HASHLIST_H
#define HASHLIST_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "bt_parse.h"
//#include "peer.h"
#include "chunk.h"
#include "packet.h"
#include "sock_io.h"

#define HASH_SIZE 20 

struct hashchunk
{
	struct sockaddr_in from;
	char hash[30];
	int chunkid;
	struct hashchunk* next;
	time_t timestamp;//used for GET requset

	Window * congWin;
	Recv_State* Recver;
};
typedef struct hashchunk hash_chunk;

typedef struct{
	int output_fd;
	int concurr_download_num;
	int max_download_num;
	int howmany_found; // show how many chunks has been found from peers
	int howmany_need; //show howmany chunks that I need to download
/******all chunks that I need, get from user GET command*****/
	hash_chunk * needlist;//MUST KICK THE DOWNLOADED CHUNK OUT OF THIS LIST!!!
/****for all GET request to send, maintain a quene,get from received IHAVE packet,should be subset of needlist****/
	hash_chunk * get_tosend_quene;
	hash_chunk * get_tosend_tail;   
/*******for all sent GET request, still waiting for data, maintain a list*****/
	hash_chunk * get_sent_list;
/******these chunks that are now receiving data*****/
	hash_chunk * downloading_list;
}Download_conn;

typedef struct 
{
	int concurr_upload_num;
	int max_upload_num;
	hash_chunk* uploading_list;
	hash_chunk* uploaded_list;//I will not give the same chunk to the same peer again!!
	//W_HD SW_list;
}Upload_conn;

void print_hash_chunks(hash_chunk *hashlist);
void append_node(hash_chunk** list,hash_chunk *node);
void delete_node_by_hash(hash_chunk** list, char* hash);
void delete_node(hash_chunk** list, hash_chunk* node);

/****move node from one list to another******/
void move_node(hash_chunk** to,hash_chunk** from,hash_chunk *node);

int length_of_list(hash_chunk *list);
/*****clear the list/quene if needed, set the list pointer to NULL*****/
int clear_hashchunk_list(hash_chunk** list);

hash_chunk* find_node(hash_chunk* list, char * hash);
hash_chunk* find_node_by_addr(hash_chunk* list,struct sockaddr_in * addr);
hash_chunk* find_prev_node(hash_chunk* list, hash_chunk *now);
hash_chunk* create_node(struct sockaddr_in* from,char *hash,int chunk_id);
/***
****put all hashchunks that I have into peer_me->haslist****
****put all hashchunks that I need into peer_me->needlist***
****/
void parse_hasneed_list(hash_chunk** list,bt_config_t *config,char *filepath);

hash_chunk* form_WhoHas(packet *pack,hash_chunk* needlist,Download_conn *down);
void form_IHav(packet *pack,packet* whohas,hash_chunk *haslist);
void form_GET(packet* get_pack, hash_chunk* hashtoget);
#endif
