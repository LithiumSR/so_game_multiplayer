
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

//Used to get ID from server
int getID(int socket_desc){
    char buf_send[BUFFERSIZE];
    char buf_rcv[BUFFERSIZE];
    IdPacket* request=(IdPacket*)malloc(sizeof(IdPacket));
    PacketHeader ph;
    ph.type=GetId;
    request->header=ph;
    request->id=-1;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return -1;
    int bytes_sent=0;
    int ret=0;
    while(bytes_sent<size){
        ret=send(socket_desc,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    Packet_free(&(request->header));
    int ph_len=sizeof(PacketHeader);
    int msg_len=0;
    while(msg_len<ph_len){
        ret=recv(socket_desc, buf_rcv+msg_len, ph_len-msg_len, 0);
        if (ret==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        msg_len+=ret;
        }
    PacketHeader* header=(PacketHeader*)buf_rcv;
    size=header->size-ph_len;

    msg_len=0;
    while(msg_len<size){
        ret=recv(socket_desc, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        msg_len+=ret;
        }
    IdPacket* deserialized_packet = (IdPacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
    printf("[Get Id] Ricevuto bytes %d \n",msg_len+ph_len);
    int id=deserialized_packet->id;
    Packet_free(&(deserialized_packet->header));
    debug_print("Assegnato ID %d \n",id);
    return id;
}

Image* getElevationMap(int socket){
    char buf_send[BUFFERSIZE];
    char buf_rcv[BUFFERSIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetElevation;
    request->header=ph;
    request->id=-1;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return NULL;
    int bytes_sent=0;
    int ret=0;

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
    }

    printf("[Elevation request] Inviati %d bytes \n",bytes_sent);
    int msg_len=0;
    int ph_len=sizeof(PacketHeader);
    printf("Size of PacketHeader is %d",ph_len);
    fflush(stdout);
    while(msg_len<ph_len){
        ret=recv(socket, buf_rcv, ph_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }

    PacketHeader* incoming_pckt=(PacketHeader*)buf_rcv;
    size=incoming_pckt->size-ph_len;
    msg_len=0;
    while(msg_len<size){
        ret=recv(socket, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }


    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
    printf("[Elevation request] Ricevuto bytes %d \n",msg_len+ph_len);
    Packet_free(&(request->header));
    Image* ris=deserialized_packet->image;
    free(deserialized_packet);
    return ris;
}


Image* getTextureMap(int socket){
    char buf_send[BUFFERSIZE];
    char buf_rcv[BUFFERSIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetTexture;
    request->header=ph;
    request->id=-1;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return NULL;
    int bytes_sent=0;
    int ret=0;
    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    printf("[Texture request] Inviati %d bytes \n",bytes_sent);
    int msg_len=0;
    int ph_len=sizeof(PacketHeader);
    while(msg_len<ph_len){
        ret=recv(socket, buf_rcv, ph_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }
    PacketHeader* incoming_pckt=(PacketHeader*)buf_rcv;
    size=incoming_pckt->size-ph_len;
    printf("[Texture Request] Size da leggere %d \n",size);
    msg_len=0;
    while(msg_len<size){
        ret=recv(socket, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret, "Cannot read from socket");
        msg_len+=ret;
    }
    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
    printf("[Texture Request] Ricevuto bytes %d \n",msg_len+ph_len);
    Packet_free(&(request->header));
    Image* ris=deserialized_packet->image;
    free(deserialized_packet);
    return ris;
}

int sendVehicleTexture(int socket,Image* texture, int id){
    char buf_send[BUFFERSIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=PostTexture;
    request->header=ph;
    request->id=id;
    request->image=texture;

    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return -1;
    int bytes_sent=0;
    int ret=0;
    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    printf("[Vehicle texture] Inviati %d bytes \n",bytes_sent);
    return 0;
}

Image* getVehicleTexture(int socket,int id){
    char buf_send[BUFFERSIZE];
    char buf_rcv[BUFFERSIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetTexture;
    request->header=ph;
    request->id=id;
    int size=Packet_serialize(buf_send,&(request->header));
    if(size==-1) return NULL;
    int bytes_sent=0;
    int ret=0;
    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    Packet_free(&(request->header));

    int ph_len=sizeof(PacketHeader);
    int msg_len=0;
    while(msg_len<ph_len){
        ret=recv(socket, buf_rcv+msg_len, ph_len-msg_len, 0);
        if (ret==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        msg_len+=ret;
        }
    PacketHeader* header=(PacketHeader*)buf_rcv;
    size=header->size-ph_len;

    msg_len=0;
    while(msg_len<size){
        ret=recv(socket, buf_rcv+msg_len+ph_len, size-msg_len, 0);
        if (ret==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        msg_len+=ret;
        }
    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len+ph_len);
    printf("[Get Vehicle Texture] Ricevuto bytes %d \n",msg_len+ph_len);
    Image* im=deserialized_packet->image;
    free(deserialized_packet);
    return im;
}

int sendGoodbye(int socket,int id){
    printf("[Goodbye] Sto per inviare qualcosa \n");
    char buf_send[BUFFERSIZE];
    IdPacket* idpckt=(IdPacket*)malloc(sizeof(IdPacket));
    PacketHeader ph;
    ph.type=PostDisconnect;
    idpckt->id=id;
    idpckt->header=ph;
    int size=Packet_serialize(buf_send,&(idpckt->header));
    printf("[Goodbye] Sto per inviare %d \n",size);
    int msg_len=0;
    while(msg_len<size){
        int ret=send(socket,buf_send+msg_len,size-msg_len,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        msg_len+=ret;
    }
    printf("[Goodbye] Bytes inviati %d \n",msg_len);
    return 0;



}




