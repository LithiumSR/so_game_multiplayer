
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <errno.h>
#include "common.h"
#include "so_game_protocol.h"
#define BUFFERSIZE 1000000

int getID(int socket_desc){
    char buf_send[BUFFERSIZE];
    char buf_rcv[BUFFERSIZE];
    IdPacket* request=(IdPacket*)malloc(sizeof(IdPacket));
    PacketHeader ph;
    ph.type=GetId;
    request->header=ph;
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
    IdPacket* deserialized_packet = (IdPacket*)Packet_deserialize(buf_rcv, msg_len);
    printf("[Get Id] Ricevuto bytes %d \n",msg_len+ph_len);
    int id=deserialized_packet->id;
    Packet_free(&(deserialized_packet->header));
    return id;
}

Image* getElevationMap(int socket){
    char buf_send[BUFFERSIZE];
    char buf_rcv[BUFFERSIZE];
    ImagePacket* request=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetElevation;
    request->header=ph;
    request->id=0;
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


    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len);
    printf("[Get Texture] Ricevuto bytes %d \n",msg_len+ph_len);
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
    request->id=0;
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
    ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, msg_len);
    printf("[Texture Request] Ricevuto bytes %d \n",msg_len+ph_len);
    Packet_free(&(request->header));
    Image* ris=deserialized_packet->image;
    free(deserialized_packet);
    return ris;
}

int sendTexture(int socket,Image* texture, int id){
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

int main(int argc, char **argv) {
    long tmp = strtol(argv[1], NULL, 0);
    if (tmp < 1024 || tmp > 49151) {
	  fprintf(stderr, "Use a port number between 1024 and 49151.\n");
	  exit(EXIT_FAILURE);
    }
    uint16_t port_number_no = htons((uint16_t)tmp); // we use network byte order
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    in_addr_t ip_addr = inet_addr("127.0.0.1");
    ERROR_HELPER(socket_desc, "Impossibile creare una socket");
	struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0
    server_addr.sin_addr.s_addr = ip_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;
	int ret= connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Impossibile connettersi al server \n");
    fprintf(stderr, "Connessione al server stabilita!\n");

    Image* vehicle_texture=Image_load("./images/arrow-right.ppm");
    if(vehicle_texture==NULL) ERROR_HELPER(-1,"Vehicle texture load failed");

    int id= getID(socket_desc);
    printf("Ricevuto id %d \n",id);
    printf("Inizio ricezione texture mappa \n");
    Image* texture_map=getTextureMap(socket_desc);
    printf("Terminata ricezione Texture Map \n");
    Image* elevation_map=getElevationMap(socket_desc);
    printf("Terminato ricezione Elevation Map \n");
    ret=sendTexture(socket_desc,vehicle_texture,id);
    printf("Terminato invio vehicle texture \n");
    return 0;
}

