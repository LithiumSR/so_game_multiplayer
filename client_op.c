
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
int send_request_type(int socket,Type tp){
    PacketHeader* ph=(PacketHeader*)malloc(sizeof(PacketHeader));
    ph->type=GetId;
    char* packet_to_send=malloc(sizeof(PacketHeader));
    int ret=Packet_serialize(packet_to_send,ph);
    if (ret==-1) ERROR_HELPER(ret,"Errore serializzazione idPacket");
    int bytes_sent=0;
    int bytes_to_send=strlen(packet_to_send);

    while(bytes_sent<bytes_to_send){
        ret=send(socket,packet_to_send+bytes_sent,bytes_to_send-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
		}
    Packet_free(ph);
    free(packet_to_send);
    return ret;
}

IdPacket* get_next_IdPacket(int socket){
    char buf[sizeof(IdPacket)]={0};
    int packet_len=sizeof(IdPacket);
    int bytes_received=0;
    int ret=0;
    while (bytes_received<packet_len) {
        ret = recv(socket, buf + bytes_received, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        bytes_received+=ret;
    }
    PacketHeader* ph=Packet_deserialize(buf,packet_len);
    IdPacket* ip= (IdPacket*)ph;
    return ip;
}

ImagePacket* get_next_ImagePacket(int socket_desc){
    char buf[sizeof(ImagePacket)]={0};
    int packet_len=sizeof(ImagePacket);
    int ret=0;
    int bytes_received=0;
    while (bytes_received<packet_len) {
        ret = recv(socket_desc, buf + bytes_received, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        bytes_received+=ret;
    }
    //More work is needed here
    PacketHeader* ph=Packet_deserialize(buf,packet_len);
    ImagePacket* ip=(ImagePacket*) ph;
    return ip;
}

int send_client_Texture(int socket, Image* my_texture, int id){
    PacketHeader ph;
    ph.type=PostTexture;
    ImagePacket* imgp= malloc(sizeof(ImagePacket));
    imgp->header=ph;
    imgp->id=id;
    imgp->image=my_texture;
    char* packet_to_send = malloc(sizeof(ImagePacket*)+sizeof(Image));
    int ret=0;
    //More work needed here

    if (ret==-1) return -1;
    int bytes_sent=0;
    int bytes_to_send=strlen(packet_to_send);

    while(bytes_sent<bytes_to_send){
        ret=send(socket,packet_to_send+bytes_sent,bytes_to_send-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
		}
    //Packet_free(ph);
    free(packet_to_send);
    return ret;
}
