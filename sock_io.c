#include "sock_io.h"
#include "win_log.h"

static FILE* fd;


/*******send the packet to every peer except myself********/
int broadcast(int sock,packet* pack, bt_peer_t *peers,int myid)
{

  bt_peer_t *curr_peer=peers;
  char *stream;

  stream=serial_pack(pack);
  
      for(;curr_peer!=NULL;curr_peer=curr_peer->next)
      {
        if(curr_peer->id==myid) continue;
         

          if(spiffy_sendto(sock, stream, pack->Pack_len, 0, (struct sockaddr *) (&(curr_peer->addr)), sizeof(struct sockaddr))!=pack->Pack_len){
              printf("\nbroadcast the packet whose type: %d  Seq: %d failed!\n",pack->Packet_Type,pack->Seq_Num);
          }
      }

      free(stream);
      return 1;

}

/******having the packet struct and dest addr,send one packet*******/
int send_pack(int mysock,packet* pack,struct sockaddr_in* toaddr)
{
    char *stream=NULL;
    stream=serial_pack(pack);
    
    if(spiffy_sendto(mysock, stream, pack->Pack_len, 0, (struct sockaddr *) toaddr, sizeof(struct sockaddr))==pack->Pack_len)     
      {
          free(stream);
          return 1;
        }
    else {
      printf("\nsend the packet whose type: %d  Seq: %d failed!\n",pack->Packet_Type,pack->Seq_Num);
      free(stream);
      return -1;
    }
}

Window* create_SW(char *hash,struct sockaddr_in* dst)
{
	Window* W=(Window*)malloc(sizeof(Window));
	bzero(W,sizeof(Window));

	memcpy(W->hash,hash,20);
	memcpy((char*)(&(W->dst)),dst,sizeof(struct sockaddr_in));

	W->LPA=0;
	W->LPS=0;
	W->LPR=1;
	W->count=1;
	W->congwin_size=1.0;
	W->ssthresh=INITIAL_CONGESTION_THRESHOLD;

	W->flowid=get_id_bytime();

	int i;
	for(i=0;i<SENDER_BUF_SIZE;i++)
	{
		W->sender_buf[i]=NULL;
	}
	WSize_log(W->flowid,W->congwin_size);
	W_verb_log(W->flowid,W->congwin_size,W->ssthresh,W->LPA,W->LPS,W->LPR,W->count,"just created");
	return W;
}


int p2p_sender(char* data,Window *SW,int len)
{
	packet* datapack=NULL;
	int addpacknum=0;
	
	int countlen=0;
	int pload_len = MAX_PACKET_SIZE - HEADER_LEN;
	char pload[MAX_PACKET_SIZE - HEADER_LEN+1]= {0};
	while(len-countlen>pload_len)
	{   
		if(SW->LPR == SENDER_BUF_SIZE-1)
		{
		/***when the sender buffer is full, stop sending***/
			return countlen;
		}

		bzero(pload,pload_len+1);
		memcpy(pload,data+countlen,pload_len);
		
		datapack=(packet*)malloc(sizeof(packet));
		bzero(datapack,sizeof(packet));

		form_DATA_pack(datapack,pload,pload_len,SW->LPR);

		SW->sender_buf[SW->LPR]=datapack;
		addpacknum++;
		SW->LPR++;
		countlen+=pload_len;
		
	}
	
	if(SW->LPR == SENDER_BUF_SIZE-1)
	{
	/***when the sender buffer is full, stop sending***/
		return countlen;
	}

	bzero(pload,pload_len+1);
	memcpy(pload,data+countlen,len-countlen);
	datapack=(packet*)malloc(sizeof(packet));
	bzero(datapack,sizeof(packet));
	form_DATA_pack(datapack,pload,len-countlen,SW->LPR);

	SW->sender_buf[SW->LPR]=datapack;
	addpacknum++;
	SW->LPR++;
	//printf("%d\n", len-countlen);
	countlen=len;

	printf("\n%d packets are added into sender window!\n",addpacknum );
	return countlen;
}

int send_onechunk(Window *SW,int offset,char *where)
{
		FILE* master_file=fopen(where,"r");
		int datafile,ret;
		char line[BT_FILENAME_LEN]={0};
		char c[5], data_file_name[BT_FILENAME_LEN];
		char data[CHUNK_SIZE+1]={0};
		if(fgets(line, BT_FILENAME_LEN, master_file) != NULL)
		{
			sscanf(line, "%s %s", c, data_file_name);
		}

		printf("data_file_name: %s\n",data_file_name);
		datafile=open(data_file_name,O_RDONLY);
		if (datafile<0)
		{
			printf("%s",strerror(datafile));
			fclose(master_file);
			return -1;
		}
		ret=pread(datafile,data,CHUNK_SIZE,offset*CHUNK_SIZE);
		printf("\nhow many bytes read %d\n",ret);
		//assert(ret==CHUNK_SIZE);

		ret=p2p_sender(data,SW,CHUNK_SIZE);
		if(ret!=CHUNK_SIZE)
		{
			printf("ERROR! only %d bytes of data sent into sender window\n", ret);
			fclose(master_file);
			close(datafile);
			return -1;
		}
		fclose(master_file);
		close(datafile);
		return 1;
}

int check_datapack_timeout(Window* sw)
{
	time_t currtime=0;
	time(&currtime);
	int index=sw->LPA+1;
	double elapse;
	if (sw->sender_buf[index]->timestamp<=0)
	{
		return 0;
	}
	 elapse=difftime(currtime,sw->sender_buf[index]->timestamp);

	/******if now new packets are sent,we need not to check timeout******/
	if (index>sw->LPS || index==1)
	{
		return 0;
	}
	else if (elapse>TIMEOUT_SEC)
	{
		int tmp=sw->congwin_size/2;
		sw->ssthresh= tmp < 2? 2: tmp ;
	/***if timeout<10 times, discard all sent(but not acked pack),set win size to 1****/
		sw->congwin_size=1;
		sw->LPS=sw->LPA;
		sw->sender_buf[sw->LPA]->ACKed_num=1;
		sw->count= sw->congwin_size - (sw->LPS - sw->LPA);
		WSize_log(sw->flowid,sw->congwin_size);
		//W_verb_log(sw->flowid,sw->congwin_size,sw->ssthresh,sw->LPA,sw->LPS,sw->LPR,sw->count,"data tm out");
		return 1;
	}
	return 0;
}


int window_sender(int mysock, Window* SW)
{
	int flag=1;
	if (SW==NULL)
	{
		return -1;
	}
	if (SW->count==0)
	{
		return 0;
	}

	int i=0;
	for(;i<SW->count;i++)
	{
		if(SW->LPS==SW->LPR-1) 
		{
			flag=0;
			break;
		}
		SW->LPS++;

		send_pack(mysock,SW->sender_buf[SW->LPS],&SW->dst);
		
		time( & (SW->sender_buf[SW->LPS]->timestamp));
	
	}
	SW->count=0;
	W_verb_log(SW->flowid,SW->congwin_size,SW->ssthresh,SW->LPA,SW->LPS,SW->LPR,SW->count,"send");
	if (flag)
	{
		if ((SW->LPS - SW->LPA) != (int32_t)SW->congwin_size)
		{
			printWindow(SW);
		}
		//assert((SW->LPS - SW->LPA) == (int32_t)SW->congwin_size);
	}
	
	return i;
}

int upon_receive_ACK(packet* ack_pack,Window* curr_win)
{
	uint32_t confirmno=ack_pack->ACK_Num;
	
    int newly_acked_no=confirmno - curr_win->LPA;
 /*****if ACK num is smaller than 1 or larger than the last-packet-sent, ignore that*******/
    if(confirmno<1 ) {
    	printf("the ACK number %dis illegal!\n",ack_pack->ACK_Num);
    	printWindow(curr_win);
    	return curr_win->congwin_size;
    }
    curr_win->sender_buf[confirmno]->ACKed_num++;
   
    if(confirmno>curr_win->LPS)
    {
    	curr_win->LPA=confirmno;
    	curr_win->LPS=curr_win->LPA;
    	curr_win->congwin_size++;
    	curr_win->count= curr_win->congwin_size - (curr_win->LPS - curr_win->LPA);
    	WSize_log(curr_win->flowid,curr_win->congwin_size);
    	W_verb_log(curr_win->flowid,curr_win->congwin_size,curr_win->ssthresh,curr_win->LPA,curr_win->LPS,curr_win->LPR,curr_win->count,"jump window");
    	return curr_win->congwin_size;
    }

	if(newly_acked_no>0){
		curr_win->LPA=confirmno;

		if(curr_win->congwin_size > curr_win->ssthresh){
			curr_win->congwin_size+= (1/curr_win->congwin_size)*newly_acked_no;
			if (curr_win->congwin_size > TCP_MAXWIN)
			{
				curr_win->congwin_size=TCP_MAXWIN;
			}
		}
		else
			curr_win->congwin_size += newly_acked_no;
		
		curr_win->count= curr_win->congwin_size - (curr_win->LPS - curr_win->LPA);

	} 
	/***handle duplicate ACK****/
	else if (newly_acked_no==0)
	{
		if (curr_win->sender_buf[confirmno]->ACKed_num>3)
		{
			int tmp=curr_win->congwin_size/2;
			curr_win->ssthresh= tmp < 2? 2: tmp ;
		/***if duplicate ACK, discard all sent(but not acked pack),set win size to 1****/
			curr_win->congwin_size=1;
			curr_win->LPS=curr_win->LPA;
			curr_win->sender_buf[curr_win->LPA]->ACKed_num=1;
			curr_win->count= curr_win->congwin_size - (curr_win->LPS - curr_win->LPA);
			WSize_log(curr_win->flowid,curr_win->congwin_size);
			W_verb_log(curr_win->flowid,curr_win->congwin_size,curr_win->ssthresh,curr_win->LPA,curr_win->LPS,curr_win->LPR,curr_win->count,"dup ACKs");
		}
		curr_win->count= curr_win->congwin_size - (curr_win->LPS - curr_win->LPA);
		
	}

	
	return curr_win->congwin_size;
}


void release_window(Window **pw)
{
	Window* w=*pw;
	int i;
	for(i=0;i<SENDER_BUF_SIZE;i++)
	{
		if (w->sender_buf[i]!=NULL)
		{
			del_packet(w->sender_buf[i]);
			w->sender_buf[i]=NULL;
		}
	}
	free(w);
	w=NULL;
}

void upon_receiving_datapack(int mysock,packet* pack, Recv_State* rev,struct sockaddr_in* addr)
{
	int32_t seq=pack->Seq_Num;
	int max= (rev->NPE+30)>512?512:(rev->NPE+30);
	if (seq<1 || seq>max)
	{
		return;
	}
	packet* ack=(packet*)malloc(sizeof(packet));
	bzero(ack,sizeof(packet)); //must clear to 0 here!!or seg fault->del_packet()
	
	if (rev->recv_buf[seq]!=NULL)
	{
		del_packet(rev->recv_buf[seq]);
	}
	
	rev->recv_buf[seq]=pack;
	
	/******to clear the last slot of the recv buffer*****/
	rev->recv_buf[SENDER_BUF_SIZE-1]=NULL;

	if (seq==rev->NPE)
	{
		int i,count=0;
		for(i=seq;i<SENDER_BUF_SIZE-1;i++)
		{			
			if (rev->recv_buf[i]==NULL)
			{
				break;
			}
			count++;

		/***send all ACK of datapack that I received,
			rather than only the largest one***/
			bzero(ack,sizeof(packet));
			form_ACK_pack(ack,i);
			send_pack(mysock,ack,addr);
			rev->recv_bytesnum+=rev->recv_buf[i]->Pack_len-rev->recv_buf[i]->Hd_len;
		}
		rev->NPE=i;
		if(rev->recv_bytesnum>500000)
			printf("now receive %d bytes\n",rev->recv_bytesnum );
		
	}
	else if(seq>rev->NPE){
		/****never send ACK 0 back*****/
		if (rev->NPE>0)
		{
			form_ACK_pack(ack,rev->NPE-1);
			send_pack(mysock,ack,addr);
		}
	}
	/***if data pack smaller than NPE(next expect),still ack the packet to sender***/
	else{
		bzero(ack,sizeof(packet));
		form_ACK_pack(ack,seq);
		send_pack(mysock,ack,addr);
	}

	del_packet(ack);
}

Recv_State* create_Recver(char* hash)
{
	Recv_State* recv=(Recv_State*)malloc(sizeof(Recv_State));
	int i;
	bzero(recv,sizeof(Recv_State));

	memcpy(recv->hash,hash,20);
	recv->NPE=1;
	recv->recv_bytesnum=0;

	for(i=0;i<SENDER_BUF_SIZE;i++)
	{
		recv->recv_buf[i]=NULL;
	}
	return recv;
}

void del_Recver(Recv_State** R)
{
	Recv_State* r=*R;
	int i;
	for(i=0;i<SENDER_BUF_SIZE;i++)
	{
		if (r->recv_buf[i]!=NULL)
		{
			del_packet(r->recv_buf[i]);
			r->recv_buf[i]=NULL;
		}
	}
	free(r);
	*R=NULL;
}


/******move the read packets' data into the outputfile after hash check*********/
/***return howmany bytes are reap*****/
int reap(Recv_State* recv,int offset,int outfd)
{
	//assert(recv->recv_packnum==recv->NPE);
	int i;
	char reap_buf[CHUNK_SIZE+1]={0};
	int buflen=0;
	int ret=0;

	size_t written=0;
	if (recv==NULL)
	{
		printf("Reaping an empty recv buffer!\n");
		return 0;
	}

	for(i=1;i<recv->NPE;i++)
	{
		memcpy(reap_buf+buflen,recv->recv_buf[i]->payload,recv->recv_buf[i]->Pack_len-recv->recv_buf[i]->Hd_len);
		buflen += recv->recv_buf[i]->Pack_len-recv->recv_buf[i]->Hd_len;
	}

	if (buflen==CHUNK_SIZE)
	{
		if(check_onechunk_hash(reap_buf,recv->hash)==1)
		{
			//outfd=open(output_file_name,O_RDWR|O_CREAT);
			printf("Check Hash Success!!\n");
			while(ret!=CHUNK_SIZE)
			{
				lseek(outfd,offset*CHUNK_SIZE+ret,SEEK_SET);
				written=write(outfd,reap_buf+ret,CHUNK_SIZE-ret);
				if (written>0)
				{
					ret+=written;
				}
				printf("retnum:%d\n",ret);
			}
			//assert(written==CHUNK_SIZE);
			return written;
		}
		printf("\nCHUNK offset: %d hash check failed!!!\n",offset);
		return -1;
	}
	return 0;
}

/*******check the hash value of the whole chunk. *****
*****compare with the GET *.chunks file item**********/
int check_onechunk_hash(char *data,char* pattern)
{
	char hash[21]={0};
	char hex[41]={0};
	shahash((uint8_t*)data, CHUNK_SIZE,(uint8_t*)hash);

	binary2hex((uint8_t*) hash, 20, hex);
	printf("the receiving hash is: %s\n", hex);
	if(memcmp(hash,pattern,20)==0) return 1;
	else return 0;
}

void check(int chunkid, Recv_State* recv)
{
	int i=1;
	Window check;
	check.LPR=1;
	printf("))))))))))))))))))))))))))))\n");
	send_onechunk(&check,chunkid,"../tmp/C.masterchunks");
	printf("(((((((((((((((((((((((((((((\n");
	fd=fopen("../tmp/check.log","a");

	for(;i<513;i++)
	{
		if(memcmp(check.sender_buf[i]->payload,recv->recv_buf[i]->payload,1024)==0)
			fprintf(fd,"(%d)p[%d][match]\t",chunkid,i );
		else{
			fprintf(fd,"(%d)p[%d][non]\t",chunkid,i );
		}

	}
	fclose(fd);
}

void printWindow(Window* curr)
{
	if(curr == NULL)
	{
		printf("    -------------------------------------\n    There is no window here!!\n");
	 	return;
	}
	char hex[41]={0};
	binary2hex((uint8_t*) curr->hash, 20, hex);
	printf("    -----------------------------------\n    Window[%d] is sending chunk:  %s\n",curr->flowid,hex);
	printf("    Communicating IP[%s]  port[%d]\n",inet_ntoa(curr->dst.sin_addr),ntohs(curr->dst.sin_port) );
	printf("    last ACKed:%d   last sent:%d   last ready to send:%d\n", curr->LPA,curr->LPS,curr->LPR);
	printf("    CongWindow size: %f Threshold: %d    How many can to send now:%d\n",curr->congwin_size,curr->ssthresh,curr->count );
}

void printRecvState(Recv_State* recv)
{
	if (recv==NULL)
	{
		printf("    ---------------------------------\n    This is not a receiver!!\n");
		return;
	}
	char hex[41]={0};
	binary2hex((uint8_t*) recv->hash, 20, hex);
	printf("    ---------------------------------\n    It is receiving chunk: %s\n",hex);
	printf("    now I have get %d packets, Next Packet Expected %d\n",recv->recv_bytesnum,recv->NPE);
}
