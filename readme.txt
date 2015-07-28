# run make to produce runables

# set spiffy_router environment in every console first:
export SPIFFY_ROUTER="127.0.0.1:12347"
./hupsim.pl -m topo.map -n /tmp/nodes.map -p 12347 -v 0 

# in different consoles, start peer nodes:
./peer -p /tmp/nodes.map -c /tmp/A.haschunks -f /tmp/C.masterchunks -m 4 -i 1


# In another console
./peer -p /tmp/nodes.map -c /tmp/A.haschunks -f /tmp/C.masterchunks -m 4 -i 2

# send GET requests to each other, e.g.:
GET /tmp/B.chunks /tmp/newB.tar

##############################################
#some detailed operations I used in my design#
##############################################

#1  I assume the receiver will receive the data packet smaller than the NPE(next packet expected), and I will send the old number of ACK back
#2  the ack number means to acknowledge the received data packet number, rather than the expected data number
#3  when the sender get the ack number but larger than the LPS( last packet sent), the window will jump to the new ack number position, and increment window size by 1. This move is to 
#   synchronize with the receiver.

###################
#File description:#
###################

# peer.c peer.h
## All the high level operation handling the abstraction concept of connections. like connection timeout, init connection, abort connection. and which to pick to establish a connection

#hashlist.c hashlist.h
## lower level of implementation: like to add a download/upload tast into the corresponding list, and remove a task out from the list when the task is finished or timeout. every list maps to the concurrent state of this tast. 

#sock_io.c sock_io.h
## bottom spiffy sender function and sliding window implementation
## broadcast function used to send whohas to every peer and sendpack function used to send a packet with structure *packet as input
## window_sender using a Window structure pointer as input to send allowed packets in the window.
## sendone_chunk send a whole chunk of 512K into the sender window buffer: fragment 512 K into 1024 bytes each
## reap function is used when all packets are successfully received. 

#packet.c packet.h
##bottom operations with packet structure. like serialize a packet and deserialized a packet. form all kinds of packets according to typical format of packet header values

#winlog.c winlog.h
##open file problem2-peer.txt. and write operation into file. the write fuction is formated input like C printf()

