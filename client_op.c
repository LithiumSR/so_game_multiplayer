
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include "common.h"
#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "so_game_protocol.h"

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int get_client_ID(int socket){
    char buf_send[1024];
    char buf_recv[1024];
    int ret;
    PacketHeader ph;
    ph.type=GetId;

    int size=Packet_serialize(buf_send, &ph);
    if (size==-1) ERROR_HELPER(ret,"Errore serializzazione idPacket");
    int bytes_sent=0;

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
		}
    int bytes_read=0;

    while(1){
        ret= recv(socket, buf_recv + bytes_read ,1,0);
        if( ret==-1 && errno==EINTR){
            continue;
            ERROR_HELPER(ret,"Errore nella recv dell'ID");
        }
        if(ret==0){
            break;
        }
        if( buf_send[bytes_read++]== '\n') break;
    }

    IdPacket* received = (ImagePacket*)Packet_deserialize(buf_recv,bytes_read);
    return received->id;
}

Image* get_image_elevation(int socket,int id){
    char buf_send[1024];
    char buf_recv[1024];

    PacketHeader ph;
    ph.type=PostElevation;

    int ret=0;
    int bytes_sent=0;
    int bytes_read=0;

    int size=Packet_serialize(buf_send, &ph);

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending map elevation request");
        if (ret==0) break;
        bytes_sent+=ret;
		}

    while (1) {
        ret = recv(socket, buf_recv + bytes_read, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Error while receiving map elevation");
        if( buf_send[bytes_read++]== '\n') break;
    }

    ImagePacket* received= (ImagePacket*)Packet_deserialize(buf_recv, bytes_read);

    return received->image;
}

Image* get_image_texture(int socket,int id){
    char buf_send[1024];
    char buf_recv[1024];

    PacketHeader ph;
    ph.type=PostTexture;
    int bytes_sent=0;
    int bytes_read=0;

    int size=Packet_serialize(buf_send, &ph);
    int ret=0;

    //More work needed here

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending map elevation request");
        if (ret==0) break;
        bytes_sent+=ret;
		}

    while (1) {
        ret = recv(socket, buf_recv + bytes_read, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Error while receiving map elevation");
        if( buf_send[bytes_read++]== '\n') break;
    }

    ImagePacket* received= (ImagePacket*)Packet_deserialize(buf_recv, bytes_read);

    return received->image;
}
