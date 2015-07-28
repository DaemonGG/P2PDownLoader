/*
 * peer.c
 *
 * Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *          Dave Andersen
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2.
 *
 */
#include "peer.h"
//#include "packet.h"
#define random(x) (rand()%x);

int main(int argc, char **argv) {

  bt_init(&config, argc, argv);
  init_winlog("problem2-peer.txt");
  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 1; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

  bt_parse_command_line(&config);
  init_peerme(&me,&config);        //init the peer_me structure
  //print_peer_me(&me);

  bt_dump_config(&config);
#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif
  peer_run(&config);
  return 0;
}


void process_inbound_udp(int sock,buf *buf) {

  struct sockaddr_in from;
  int recv;
  socklen_t fromlen;

  fromlen = sizeof(from);
  recv=spiffy_recvfrom(sock, buf->buf+buf->buf_len, BUFLEN2 - buf->buf_len, 0, (struct sockaddr *) &from, &fromlen);
  buf->buf_len+=recv;
  
  while(buf->buf_len>=16)
  {
      packet *newcome_pack=(packet*)malloc(sizeof(packet));
      bzero(newcome_pack,sizeof(packet));
      get_firstpack(buf,newcome_pack);

  /**process received paceket here***/
      dealwithincomepack(newcome_pack,&from);
  /**********************************/

      
  }

}

void process_get(char *chunkfile, char *outputfile) {
  printf("PROCESS GET SKELETON CODE CALLED.   (%s, %s)\n", chunkfile, outputfile);
  reset_mydownload(&me,chunkfile,outputfile);
  printf("\nINitialize this new Download successfully!!\n");
  //WhoHas_sender(me.mydownload.needlist,config.peers);  //iteratively send needlist, 50 each packet
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {

      process_get(chunkf, outf);

      printf("Asking for resourses......\n");
    }
  }
}


void peer_run(bt_config_t *config) {
  //int sock;
  hash_chunk* currupload;
  hash_chunk* currdownload;
  struct sockaddr_in myaddr;
  fd_set readfds;
  struct user_iobuf *userbuf;
  struct timeval select_tm={0,1000};//wait 1 millisecond for select

  buf recv_buf; //buffer used to buf all received raw packets
  bzero(&recv_buf,sizeof(buf));
  
  if ((userbuf = create_userbuf()) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(-1);
  }
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }
  
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  inet_aton("127.0.0.1", &myaddr.sin_addr);
  //myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);
  
  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }
  
  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
  
  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    
    nfds = select(sock+1, &readfds, NULL, NULL,&select_tm);
    
    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds)) {
	       process_inbound_udp(sock,&recv_buf);
      }
      
      if (FD_ISSET(STDIN_FILENO, &readfds)) {
	       process_user_input(STDIN_FILENO, userbuf, handle_user_input,
			   "Currently unused");
      }
    }

      check_timeout(&(me.mydownload.get_sent_list));
      if(GET_sender(&me.mydownload)==0 
        && me.mydownload.howmany_found<me.mydownload.howmany_need
        && me.mydownload.max_download_num>me.mydownload.concurr_download_num 
        && me.mydownload.concurr_download_num==0 )
      {
        /*****periodically retransmit the whohas pack*****/
        WhoHas_sender(me.mydownload.needlist,config->peers);  //iteratively send needlist, 50 each packet
      }
      

  /*****check data pack timeout first for every window,then send the packets*******/
      currupload=me.myupload.uploading_list;
      while(currupload!=NULL)
      {    

          if (currupload->congWin->LPS < currupload->congWin->LPR-1)
          {
            window_sender(sock,currupload->congWin);
          }

          /****here means the upload of one chunk finished!*****/
          if(currupload->congWin->LPA== currupload->congWin->LPR-1 )          
          {
            hash_chunk* tmp=currupload->next;
            
            move_node(& me.myupload.uploaded_list,& me.myupload.uploading_list,currupload);

            me.myupload.concurr_upload_num--;
            currupload=tmp;

              continue;
          }

          check_datapack_timeout(currupload->congWin);
          check_recvack_timeout(currupload);

          currupload=currupload->next;
      }

      currdownload=me.mydownload.downloading_list;
      for(;currdownload!=NULL;currdownload=currdownload->next)
      {
          if (currdownload->Recver->recv_bytesnum==CHUNK_SIZE)
          {
            printf("intotal recv %d bytes\n", currdownload->Recver->recv_bytesnum);
            
           /*******get all datapack payload out, merge and check hash, then put into file******/
            reap(currdownload->Recver,currdownload->chunkid,me.mydownload.output_fd);
            
            /****free all resources of this download*****/
            finish_one_download(currdownload);

            printf("down load chunk finished!!!!!!!!!!!!!!\n");
            if (me.mydownload.needlist==NULL)
            {
                complete_mydownload(& me.mydownload);
                printf("GOT file!\n");
                break;
            }
          }
          if (check_recvdata_timeout(currdownload)==1)
          {
             printf("Timeout! Cancel one download!\n");
          }

      }

    }
}

/******init the structure peer_me, which contains** 
********the whole scope of current prre************/
void init_peerme(peer_me *me,bt_config_t *config)
{
  bzero(me, sizeof(peer_me));
  me->id=config->identity;
  
  me->addr.sin_family=AF_INET;
  inet_aton("127.0.0.1", &me->addr.sin_addr);
  me->addr.sin_port=htons(config->myport);

  //me->concurr_upload_num=0;

  me->mydownload.concurr_download_num=0;
  me->mydownload.max_download_num=config->max_conn;
  me->mydownload.needlist=NULL;             //filled after received GET command
  me->mydownload.get_tosend_quene=NULL;
  me->mydownload.get_tosend_tail=NULL;
  me->mydownload.get_sent_list=NULL;
  me->mydownload.downloading_list=NULL;

  //me->myupload.SW_list.whd=NULL;
  //me->myupload.SW_list.window_num=0;
  me->myupload.max_upload_num=config->max_conn;
  me->myupload.concurr_upload_num=0;
  me->myupload.uploading_list=NULL;
  me->myupload.uploaded_list=NULL;
  parse_hasneed_list(&(me->haslist),config,NULL);
}

/**********printf all peer_me elements, for debug*********/
void print_peer_me(peer_me* me)
{
  printf("\n\nmy peer id: %d\ncurrent upload num: %d\ncurrent down load num:%d\nmy address:%s\nmy port:%d\n",
    me->id,me->myupload.concurr_upload_num,me->mydownload.concurr_download_num,inet_ntoa(me->addr.sin_addr),ntohs(me->addr.sin_port));
  printf("*********haslist elements:****************\n");
  print_hash_chunks(me->haslist);

  printf("*********needlist elements:***************\n");
  print_hash_chunks(me->mydownload.needlist);

  printf("*********get_tosend_quene elements:*******\n");
  print_hash_chunks(me->mydownload.get_tosend_quene);

  printf("*********get_sent_list elements:**********\n");
  print_hash_chunks(me->mydownload.get_sent_list);

  printf("*********downloading elements list:**********\n");
  print_hash_chunks(me->mydownload.downloading_list);

  printf("*********uploading elements list:**********\n");
  print_hash_chunks(me->myupload.uploading_list);

  printf("*********uploaded elements list:**********\n");
  print_hash_chunks(me->myupload.uploaded_list);

}


void dealwithincomepack(packet *pack,struct sockaddr_in *toaddr)
{
  switch(pack->Packet_Type)
  {
    /**WhoHas***/
    case 0:{
          IHav_sender(pack,me.haslist,toaddr);
          del_packet(pack);
          break;
        }
    /***IHav****/
    case 1:{
          fill_toget_list(pack,&(me.mydownload.get_tosend_quene),toaddr);

          del_packet(pack);
          break;
        }
    /****GET***/
    case 2:{
          if(me.myupload.concurr_upload_num<me.myupload.max_upload_num 
            && find_node_by_addr(me.myupload.uploading_list,toaddr)==NULL)
          {
            printf("receive GET!!\n");
              hash_chunk* if_uploaded=find_node_by_addr(me.myupload.uploaded_list,toaddr);
              if(if_uploaded==NULL)
              {
                  init_upload(pack,toaddr);
              
              }
              else{
                  if(memcmp(if_uploaded->hash,pack->payload,20)!=0)
                  {
                      init_upload(pack,toaddr);
                      
                  }
              }
          }
          del_packet(pack);
          break;
        }
    /***DATA***/
    case 3:{

          hash_chunk *conn=find_node_by_addr( me.mydownload.downloading_list, toaddr);
        
          if(conn)
          {
            /******every useful data pack come, refresh timestamp******/
            time(& conn->timestamp);     
            upon_receiving_datapack(sock,pack,conn->Recver,toaddr);
          }
          else 
          {
            conn=find_node_by_addr( me.mydownload.get_sent_list, toaddr);
            if(conn!=NULL)
            {
                init_download(conn);
                print_hash_chunks(conn);
                print_pack(pack);
                upon_receiving_datapack(sock,pack,conn->Recver,toaddr);
                //print_peer_me(&me);
            }
          }
          break;
        }
    /***ACK****/
    case 4:{
       
        hash_chunk *conn=find_node_by_addr( me.myupload.uploading_list, toaddr);
        if (conn!=NULL)
        {

          time(& conn->timestamp);
          upon_receive_ACK(pack,conn->congWin);
         
        }
        else{
          printf("\nACK arrive,but cannot find the corresponding connection!!!\n");
        }
        del_packet(pack);
        break;
    }
   // default:
  }
}
/**
****iteratively send whohas packets 
****with every 50 hashes in its payload
**/
void WhoHas_sender(hash_chunk *needlist,bt_peer_t *peers)
{
  hash_chunk* curr=needlist;
  packet *whohas_pack;
  while(curr!=NULL)
  {
    /****make a whohas packet according to haslist****/
      whohas_pack=(packet*)malloc(sizeof(packet));
      //bzero(whohas_pack,sizeof(packet));
      curr=form_WhoHas(whohas_pack,curr,&(me.mydownload));
    /**************************************************/
      if(whohas_pack->Chunkhash_Num==0)
      {
        printf("\nthe whohas pack has nothing to ask for..don't broadcast\n");
        del_packet(whohas_pack);
        whohas_pack=NULL;
        continue;
      }
      broadcast(sock,whohas_pack, peers,me.id);
      del_packet(whohas_pack);
      whohas_pack=NULL;
  }
}

/*
***send I hav pack when I receive WhoHas pack, 
***if I dont have the chunk,send nothing back
*/
void IHav_sender(packet* WhoHas_pack,hash_chunk* haslist,struct sockaddr_in *toaddr)
{
    packet *ihav_pack;
    ihav_pack=(packet *)malloc(sizeof(packet));
    bzero(ihav_pack,sizeof(packet));

    form_IHav(ihav_pack,WhoHas_pack,haslist);
/***if I dont have what the peer asked for, then send nothing back***/
    if (ihav_pack->Chunkhash_Num>0)
    {
      send_pack(sock,ihav_pack,toaddr);
    }
    
}

/*****after receive IHav pack, decide which hash need to 
*****add to get_tosend_list*****/
void fill_toget_list(packet *ihav_pack,hash_chunk** get_tosend,struct sockaddr_in *from_addr)
{
  char find_hash[30]={0};
  int count=0;
  hash_chunk* node=NULL;
  hash_chunk* if_ineed=NULL;
  hash_chunk* if_stored=NULL;
  for(;count<ihav_pack->Chunkhash_Num;count++)
  {
      bzero(find_hash,30);
      memcpy(find_hash,ihav_pack->payload+count*HASH_SIZE,HASH_SIZE);
      if_ineed=find_node(me.mydownload.needlist,find_hash);
      if_stored=find_node(me.mydownload.get_tosend_quene,find_hash);

      if( find_node(me.mydownload.downloading_list,find_hash)==NULL
        && find_node(me.mydownload.get_sent_list,find_hash)==NULL
        && if_ineed!=NULL)        //this chunk must be what I need!! or why GET it?!
      {
        if (if_stored==NULL)
        {
            node=create_node(from_addr,find_hash,if_ineed->chunkid); //where is the chunk id?
            append_node(get_tosend,node);
            me.mydownload.howmany_found++;
        }
        else{
          if( ntohs(from_addr->sin_port)!=ntohs(if_stored->from.sin_port) &&
              (find_node_by_addr(me.mydownload.downloading_list,& if_stored->from)!=NULL 
                || find_node_by_addr(me.mydownload.get_sent_list,& if_stored->from)!=NULL)
            )
          {
            printf("move to another peer port[%d]\n", ntohs(from_addr->sin_port));
              delete_node(get_tosend,if_stored);
              node=create_node(from_addr,find_hash,if_ineed->chunkid); //where is the chunk id?
              append_node(get_tosend,node);
          }
        
        }
      }
  }

}

/***check the get_sent_list to see any GET request time out, if timeout for more than 3 times, drop the GET request***/
/***return the number of the tomeout chunks***/
int check_timeout(hash_chunk** list)
{
  int count=0;
  hash_chunk* curr=*list;
  hash_chunk* tmp;
  time_t currtm=0;
  currtm=time(NULL);
  double elapse;

  while(curr!=NULL)
  {
    elapse=difftime(currtm,curr->timestamp);
    /***GET request timeout 10 sec, drop this****/
    if(elapse>5*TIMEOUT_SEC){
      tmp=curr->next;
      delete_node(list,curr);
      me.mydownload.concurr_download_num--;
      me.mydownload.howmany_found--;
      count++;
      curr=tmp;
      continue;
    }
    curr=curr->next;
  }

  return count;
}

/***send new GET request,return the*********
******number of the GET packs just sent***/
int GET_sender(Download_conn* down)
{
   int curr_down=0;
   int max_send=down->max_download_num - down->concurr_download_num;
   if (max_send==0)
   {
     return 0;
   }

   curr_down=pick_tosend_GET(down,max_send);

   down->concurr_download_num+=curr_down;
   return curr_down;
}

/*****move qualified nodes from get_tosend_list  to  get_sent_list*******/
int pick_tosend_GET(Download_conn* down,int maxpick_num)
{
  hash_chunk* curr=down->get_tosend_quene;
  hash_chunk* sent_list=down->get_sent_list;
  hash_chunk* download_list=down->downloading_list;
  packet * get_pack;

  int count=0;
  while(curr!=NULL && count<maxpick_num)
  {
    if(find_node_by_addr(sent_list,&(curr->from))==NULL
      && find_node_by_addr(download_list,&(curr->from))==NULL)
    {
     
      move_node(& down->get_sent_list,& down->get_tosend_quene,curr);

      time(&curr->timestamp);

      get_pack=(packet*)malloc(sizeof(packet));
      form_GET(get_pack,curr);

      send_pack(sock,get_pack,&(curr->from));
      del_packet(get_pack);
      count++;
    }
    
    curr=curr->next;
  }
  return count;
}

void reset_mydownload(peer_me* me,char* chunkfile, char* outputfile)
{
    me->mydownload.concurr_download_num=0;
    me->mydownload.max_download_num=config.max_conn;
    me->mydownload.howmany_found=0;
    bzero(config.output_file,BT_FILENAME_LEN);
    /*****clear the need list, set the list pointer to NULL*****/
    clear_hashchunk_list(&(me->mydownload.needlist));
    clear_hashchunk_list(&(me->mydownload.get_tosend_quene));
    clear_hashchunk_list(&(me->mydownload.get_sent_list));
    clear_hashchunk_list(&(me->mydownload.downloading_list));

    /*****put every piece I want GET into needlist******/ 
    parse_hasneed_list(&(me->mydownload.needlist),&config,chunkfile);

    me->mydownload.howmany_need=length_of_list(me->mydownload.needlist);
    strncpy(config.output_file,outputfile,strlen(outputfile)); 
    me->mydownload.output_fd=open(outputfile,O_RDWR|O_CREAT,0777);
}

void complete_mydownload(Download_conn* down)
{
    close(down->output_fd);
    down->max_download_num=0;
   /*****clear the need list, set the list pointer to NULL*****/
    //clear_hashchunk_list(&(me->mydownload.needlist));
    clear_hashchunk_list(&(down->get_tosend_quene));
    clear_hashchunk_list(&(down->get_sent_list));
    clear_hashchunk_list(&(down->downloading_list));
}
int init_upload(packet* getpack,struct sockaddr_in* dst)
{
    char hash[21]={0};
    memcpy(hash,getpack->payload,HASH_SIZE);
    //Window* sw;
    hash_chunk* new;
    me.myupload.concurr_upload_num++;

    hash_chunk* uploading_chunk=find_node(me.haslist,hash);
    if (uploading_chunk!=NULL)
    {
        new=create_node(dst,hash,uploading_chunk->chunkid);
        time(& new->timestamp);
        append_node(&me.myupload.uploading_list,new);
    
        new->congWin=create_SW(hash,dst);
        printWindow(new->congWin);
        send_onechunk(new->congWin,uploading_chunk->chunkid,config.chunk_file);
        return 1;
    }  

    printf("\nCan not upload a chunk that I don't have!!\n");
    return -1;
}

void abort_upload(hash_chunk* todel)
{

    if (todel==NULL)
    {
       fprintf(stderr, "\nthe to-abort upload-connection is not in the uploading list!!\n" );
        return;
    }
    delete_node(& (me.myupload.uploading_list), todel);

    me.myupload.concurr_upload_num--;
}

void init_download(hash_chunk* down_conn)
{
    move_node(& (me.mydownload.downloading_list),& (me.mydownload.get_sent_list),down_conn);
    time(& down_conn->timestamp);
    down_conn->Recver=create_Recver(down_conn->hash);
}

void finish_one_download(hash_chunk* todel)
{
    if (todel==NULL)
    {
       fprintf(stderr, "\nthe to-cancel download-connection is not in the downloading list!!\n" );
        return;
    }
    delete_node_by_hash(&(me.mydownload.needlist),todel->hash);
    delete_node(&(me.mydownload.downloading_list),todel);

    //bzero(config.output_file,BT_FILENAME_LEN);
    me.mydownload.concurr_download_num--;
    //print_peer_me(&me);
}

/****check every download timeout, if 10 sec no any data come,
**** drop this connection*****/
int check_recvdata_timeout(hash_chunk* curr)
{
    time_t currtm;
    time(&currtm);
    double elapse=difftime(currtm,curr->timestamp);
    if (elapse>5*TIMEOUT_SEC)
    {
        delete_node(&(me.mydownload.downloading_list),curr);
        me.mydownload.howmany_found--;
        me.mydownload.concurr_download_num--;
        return 1;
    }
    return 0;
}

/****check every upload timeout, if 16 sec no any ACK come,
**** drop this connection*****/
int check_recvack_timeout(hash_chunk* curr)
{
    time_t currtm;
    time(&currtm);
    double elapse=difftime(currtm,curr->timestamp);
    if (elapse>8*TIMEOUT_SEC)
    {
      printf("abort upload after elapse%f\n",elapse);
        abort_upload(curr);
        return 1;
    }
    return 0;
}