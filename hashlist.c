#include "hashlist.h"

void print_hash_chunks(hash_chunk *hashlist)
{
	hash_chunk *curr=hashlist;
	char hex[41]={0};
	int seq_num=0;
	if (hashlist==NULL)
	{
		printf("\nThis list is empty!!\n");
		return;
	}
	
	for(;curr!=NULL;curr=curr->next)
	{
		bzero(hex,41);
		binary2hex((uint8_t*) curr->hash, 20, hex);
		printf("\n%d  this chunk should from:[ip]%s\t[port]%d\n    chunk ID: %d\n    Timestamp: %d\n    hash:%s\n",seq_num,inet_ntoa(curr->from.sin_addr),ntohs(curr->from.sin_port),curr->chunkid,(int)curr->timestamp,hex);
		printWindow(curr->congWin);
		printRecvState(curr->Recver);
		seq_num++;
	}
	return;
}
void append_node(hash_chunk** list,hash_chunk *node)
{
	if(node!=NULL)
	{
		if((*list)==NULL){
			*list=node;
			node->next=NULL;
		}
		else{
			node->next=(*list);
			(*list)=node;
			//node->next=NULL;
		}
	}
}
void delete_node(hash_chunk** list, hash_chunk* node)
{
	//hash_chunk *curr=find_node(*list,hash);
	if (node==NULL)
	{
		printf("\nYOU CANNOT DLETE AN NULL NODE!\n");
		return;
	}
	if(*list==NULL)
	{
		printf("deleting from empty list!\n");
		return;
	}
	else if (*list==node)
	{
		*list=node->next;
		if (node->congWin!=NULL)
		{
			release_window(& node->congWin);
		}
		if (node->Recver!=NULL)
		{
			del_Recver(& node->Recver);
		}
		free(node);
		//*list=NULL;
	}
	else{
		hash_chunk *prev=find_prev_node(*list,node);
		prev->next=node->next;
		if (node->congWin!=NULL)
		{
			release_window(& node->congWin);
		}
		if (node->Recver!=NULL)
		{
			del_Recver(& node->Recver);
		}
		free(node);
	}
}

void delete_node_by_hash(hash_chunk** list, char* hash)
{
	hash_chunk *curr=find_node(*list,hash);
	if (*list==curr)
	{
		*list=curr->next;
		if (curr->congWin!=NULL)
		{
			release_window(&curr->congWin);
		}
		if (curr->Recver!=NULL)
		{
			del_Recver(& curr->Recver);
		}
		free(curr);
		//*list=NULL;
	}
	else{
		hash_chunk *prev=find_prev_node(*list,curr);
		prev->next=curr->next;
		if (curr->congWin!=NULL)
		{
			release_window(&curr->congWin);
		}
		if (curr->Recver!=NULL)
		{
			del_Recver(& curr->Recver);
		}
		free(curr);
	}
	if (length_of_list(*list)==0)
	{
		*list=NULL;
	}
}
/****move node from one list to another******/
void move_node(hash_chunk** to,hash_chunk** from,hash_chunk *node)
{
		//printf("\nin movenode: this chunk should from:[ip]%s\t[port]%d\nchunk ID: %d\n",inet_ntoa(node->from.sin_addr),ntohs(node->from.sin_port),node->chunkid);
	if (node==NULL)
	{
		printf("\nYOU CANNOT MOVE AN NULL NODE!\n");
		return;
	}
	if(*from==NULL)
	{
		printf("you cannot moving out from empty list!\n");
		return;
	}
	else if (*from==node)
	{
		*from=node->next;
		//free(node);
		//*list=NULL;
	}
	else{
		hash_chunk *prev=find_prev_node(*from,node);
		prev->next=node->next;
		//free(node);
	}

	append_node(to,node);
	//printf("\n.........move one node into the list\n");
	//print_hash_chunks(*to);
	//delete_node(from,node);


}

hash_chunk* find_node(hash_chunk* list, char * hash)
{
	hash_chunk *curr=list;
	for(;curr!=NULL;curr=curr->next)
	{
		if(memcmp(curr->hash,hash,20)==0)
			break;
	}
	return curr;
}

hash_chunk* find_node_by_addr(hash_chunk* list,struct sockaddr_in * addr)
{
	hash_chunk *curr=list;
	for(;curr!=NULL;curr=curr->next)
	{
		if(strcmp(inet_ntoa(curr->from.sin_addr),inet_ntoa(addr->sin_addr))==0 
			&& ntohs(curr->from.sin_port)==ntohs(addr->sin_port))
			break;
	}
	return curr;
}

hash_chunk* find_prev_node(hash_chunk* list, hash_chunk *now)
{
	hash_chunk *curr=list;
	for(;curr!=NULL;curr=curr->next)
	{
		if(curr->next==now)
			break;
	}
	return curr;
}

hash_chunk* create_node(struct sockaddr_in* from,char *hash,int chunk_id)
{
	 hash_chunk* newnode=(hash_chunk*)malloc(sizeof(hash_chunk));
	 bzero(newnode,sizeof(hash_chunk));
/*
	 newnode->from.sin_family = AF_INET;
	// inet_aton("127.0.0.1", &newnode->addr.sin_addr);
	 newnode->from.sin_addr=from->sin_addr;
	 newnode->from.sin_port=from->sin_port;
*/
	 memcpy(& (newnode->from),from,sizeof(struct sockaddr_in));

	 memcpy(newnode->hash,hash,20);
	 newnode->chunkid=chunk_id;
	 newnode->timestamp=0;

//	 printf("create new node:\n[ip]%s\n[port]%d\nchunk ID: %d\nhash:%s\n",
//			inet_ntoa(newnode->from.sin_addr),ntohs(newnode->from.sin_port),newnode->chunkid,newnode->hash);

	 return newnode;
}

int length_of_list(hash_chunk *list)
{
	int count=0;
	if (list!=NULL)
	{
		for(;list!=NULL;list=list->next)
			count++;
	}

	return count;
}

/*****clear the list/quene if needed, set the list pointer to NULL*****/
int clear_hashchunk_list(hash_chunk** list)
{
	int count=0;

	if(list==NULL) return count;

	hash_chunk* curr=*list;
	hash_chunk* next=NULL;
	while(curr!=NULL)
	{
		next=curr->next;
		delete_node(list,curr);
		curr=next;
		count++;
	}
	*list=NULL;
	return count;
}

/***
****put all hashchunks that I have into peer_me->haslist****
****put all hashchunks that I need(GET command) into peer_me->needlist***
****/
void parse_hasneed_list(hash_chunk** list,bt_config_t *config,char *filepath)
{
  FILE* f;
  hash_chunk* node;
  struct sockaddr_in myaddr;
  int chunkid;
  char line[BT_FILENAME_LEN],hash[50];
  bzero(line,BT_FILENAME_LEN);
  if (config!=NULL)
  {
    myaddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &myaddr.sin_addr);
    myaddr.sin_port = htons(config->myport);
    //myaddr.sin_port = htons(8888);
  }

    if (filepath==NULL)
    {
      f = fopen(config->has_chunk_file, "r");
    }
    else{
      f = fopen(filepath, "r");
    }
  //  f = fopen("../tmp/A.haschunks", "r");

  assert(f != NULL);

  char decoded_hash[30]={0};
  while (fgets(line, BT_FILENAME_LEN, f) != NULL) 
  {
      if (line[0] == '#') continue;
      bzero(decoded_hash,30);
      bzero(hash,50);
      assert(sscanf(line, "%d %s", &chunkid, hash) != 0);

      hex2binary(hash,40,(uint8_t*)(decoded_hash));
      node=create_node(&myaddr,decoded_hash,chunkid);
      assert(node != NULL);
      append_node(list,node);
  }
  //  print_hash_chunks(me->haslist);
  fclose(f);

}

hash_chunk* form_WhoHas(packet *pack,hash_chunk* needlist,Download_conn *down)
{
	//int len=length_of_list(down->needlist);
	//if(len>50)
	//	len = 50;
	char pload[MAX_HASHNUM_PER_PACK*HASH_SIZE+1]={0};

	bzero(pack,sizeof(packet));
	hash_chunk* curr=needlist;

	pack->Packet_Type=0;
	pack->Hd_len=HEADER_LEN;
	
	pack->Magic_Num=15441;
	pack->Version_Num=1;	
	pack->ACK_Num=0;
	pack->Seq_Num=0;
		
	int count;
	for(count=0;curr!=NULL && count<MAX_HASHNUM_PER_PACK;curr=curr->next)
	{
		if(find_node(down->get_tosend_quene,curr->hash)==NULL
			&& find_node(down->get_sent_list,curr->hash)==NULL
			&& find_node(down->downloading_list,curr->hash)==NULL)
		{
			memcpy(pload+count*HASH_SIZE,curr->hash,HASH_SIZE);
			count++;
		}		
	}
	pack->Chunkhash_Num=count;
	pack->Pack_len=pack->Hd_len+ count*HASH_SIZE+4; //header lenth+2 hash+"hashnum pad pad pad"
	pack->payload=(char*)malloc(pack->Chunkhash_Num*HASH_SIZE+1);
	bzero(pack->payload,pack->Chunkhash_Num*HASH_SIZE+1);
	memcpy(pack->payload,pload,count*HASH_SIZE);
	return curr;
}

void form_IHav(packet *pack,packet* whohas,hash_chunk *haslist)
{
	int len=0;
	char pload[MAX_HASHNUM_PER_PACK*HASH_SIZE+1]={0};
	bzero(pack,sizeof(packet));
	//hash_chunk* curr=haslist;
	pack->Packet_Type=1;
	pack->Hd_len=HEADER_LEN;
	pack->Magic_Num=15441;
	pack->Version_Num=1;
	
	pack->ACK_Num=0;
	pack->Seq_Num=0;

	int count=0;
	char find_hash[30]={0};

	for(;count<whohas->Chunkhash_Num;count++)
	{
		memcpy(find_hash,whohas->payload+count*HASH_SIZE,HASH_SIZE);
		if(find_node(haslist, find_hash)!=NULL)
		{
			memcpy(pload+len*HASH_SIZE,find_hash,HASH_SIZE);
			len++;
		}
			
	}
	pack->Chunkhash_Num=len;
	pack->payload=(char*)malloc(len*HASH_SIZE+1);
	bzero(pack->payload,len*HASH_SIZE+1);
	memcpy(pack->payload,pload,len*HASH_SIZE);
	pack->Pack_len=pack->Hd_len+ len*HASH_SIZE+4; //header lenth+2 hash+"hashnum pad pad pad"
}


void form_GET(packet* get_pack, hash_chunk* hashtoget)
{
	bzero(get_pack,sizeof(packet));
	//hash_chunk* curr=haslist;
	get_pack->Packet_Type=2;
	get_pack->Hd_len=HEADER_LEN;
	get_pack->Magic_Num=15441;
	get_pack->Version_Num=1;
	
	get_pack->ACK_Num=0;
	get_pack->Seq_Num=1; //with meaning
	get_pack->Pack_len=HEADER_LEN+HASH_SIZE;
	get_pack->payload=(char*)malloc(HASH_SIZE+1);
	bzero(get_pack->payload,HASH_SIZE+1);
	memcpy(get_pack->payload,hashtoget->hash,HASH_SIZE);	

	get_pack->Chunkhash_Num=0; //meaningless!
}

