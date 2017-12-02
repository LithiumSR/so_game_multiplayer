
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

// converts a well formed packet into a string in dest.
// returns the written bytes
// h is the packet to write
int send_request_type(int socket,Type tp){
    PacketHeader* ph=malloc(sizeof(PacketHeader));
    ph->type=Type.GetId;
    char* packet_to_send;

    int ret=Packet_serialize(packet_to_send,ph);
    if (ret==-1) return -1;
    int bytes_sent=0;
    int bytes_to_send=strlen(packet_to_send);

    while(bytes_sent<bytes_to_send){
        ret=send(socket_desc,packet_to_send+bytes_sent,bytes_to_send-bytes_sent,0);
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
    packet_len=sizeof(IdPacket);
    while (bytes_received<packet_len) {
        ret = recv(socket_desc, buf + msg_len, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        bytes_received+=ret;
    }
    IdPacket* ph=Packet_deserialize(buf,packet_len);
    return ph
}

ImagePacket* get_next_ImagePacket(int socket){
    char buf[sizeof(ImagePacket)]={0};
    packet_len=sizeof(ImagePacket);
    while (bytes_received<packet_len) {
        ret = recv(socket_desc, buf + msg_len, 1, 0);
        if (ret == -1 && errno == EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        bytes_received+=ret;
    }
    //More work is needed here
    ImagePacket* ph=Packet_deserialize(buf,packet_len);
    return ph
}

int send_client_Texture(int socket, Image* my_texture, int id){
    PacketHeader ph;
    ph.type=Type.PostTexture;
    ph.id
    ImagePacket* imgp= malloc(sizeof(ImagePacket));
    imgp->header=ph;
    imgp->id=id;
    imgp->image=my_texture;
    char* packet_to_send;
    //More work needed here

    if (ret==-1) return -1;
    int bytes_sent=0;
    int bytes_to_send=strlen(packet_to_send);

    while(bytes_sent<bytes_to_send){
        ret=send(socket_desc,packet_to_send+bytes_sent,bytes_to_send-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
		}
    Packet_free(ph);
    free(packet_to_send);
    return ret;
}
