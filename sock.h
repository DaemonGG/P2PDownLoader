#ifndef SOCK_IO_H
#define SOCK_IO_H

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "spiffy.h"
#include "hashlist.h"
#include "packet.h"
#include "bt_parse.h"
#include "peer.h"
/**send the packet to all peers in the 'peers' list***/
int broadcast(packet* pack, bt_peer_t *peers);

/******having the packet struct and dest addr,send one packet*******/
int send_pack(int mysock,packet* pack,struct sockaddr_in* toaddr);
#endif
