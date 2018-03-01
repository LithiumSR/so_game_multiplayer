
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
int get_client_ID(int socket){
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];
    int ret;
    PacketHeader ph;
    IdPacket* idpckt=(IdPacket*)malloc(sizeof(IdPacket));
    ph.type=GetId;
    idpckt->id=-1;
    idpckt->header=ph;
    debug_print("Creato pacchetto");

    int size=Packet_serialize(buf_send, &(idpckt->header));
    if (size==-1) ERROR_HELPER(-1,"Packet serialize didn't worked on ID packet");
    int bytes_sent=0;

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Errore invio");
        if (ret==0) break;
        bytes_sent+=ret;
		}
    int bytes_read=0;
    Packet_free(&(idpckt->header));
    while(1){
        ret= recv(socket, buf_recv,BUFFSIZE,0);
        if( ret==-1 && errno==EINTR){
            continue;
            ERROR_HELPER(ret,"Errore nella recv dell'ID");
        }
        if(ret==0){
            break;
        }
        else break;
    }
    IdPacket* received = (IdPacket*)Packet_deserialize(buf_recv,bytes_read);
    debug_print("Ricevuto %d \n",received->id);
    return received->id;
}

//Used to get elevation map from the server
Image* get_image_elevation(int socket,int id){
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];

    PacketHeader ph;
    ImagePacket* imagepckt=(ImagePacket*)malloc(sizeof(ImagePacket));
    ph.type=GetElevation;
    imagepckt->id=id;
    imagepckt->header=ph;

    int ret=0;
    int bytes_sent=0;
    int bytes_read=0;

    int size=Packet_serialize(buf_send, &(imagepckt->header));

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending map elevation request");
        if (ret==0) break;
        bytes_sent+=ret;
		}
    Packet_free(&(imagepckt->header));

     while(1){
        ret= recv(socket, buf_recv,BUFFSIZE,0);
        if( ret==-1 && errno==EINTR){
            continue;
            ERROR_HELPER(ret,"Failed receive of image elevation");
        }
        if(ret==0){
            break;
        }
        else break;
    }

    ImagePacket* received= (ImagePacket*)Packet_deserialize(buf_recv, bytes_read);

    return received->image;
}

//Used to get texture image from server
Image* get_image_texture(int socket,int id){
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];

    PacketHeader ph;
    ImagePacket* imagepckt=(ImagePacket*)malloc(sizeof(ImagePacket));
    ph.type=GetTexture;
    imagepckt->id=id;
    imagepckt->header=ph;
    int bytes_sent=0;
    int bytes_read=0;

    int size=Packet_serialize(buf_send, &(imagepckt->header));
    if (size==-1) ERROR_HELPER(-1,"Packet serialize didn't worked on image packet");
    int ret=0;

    //More work needed here

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending map elevation request");
        if (ret==0) break;
        bytes_sent+=ret;
		}
    Packet_free(&(imagepckt->header));

     while(1){
        ret= recv(socket, buf_recv,BUFFSIZE,0);
        if( ret==-1 && errno==EINTR){
            continue;
            ERROR_HELPER(ret,"Failed receive of image texture");
        }
        if(ret==0){
            break;
        }
        else break;
    }

    ImagePacket* received= (ImagePacket*)Packet_deserialize(buf_recv, bytes_read);

    return received->image;
}

//Used to get a vehicle texture given an id
Image* get_vehicle_texture(int socket, int id){
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];
    int ret;
    int bytes_read=0;
    int bytes_sent=0;

    PacketHeader ph;
    ImagePacket* imagepckt=(ImagePacket*)malloc(sizeof(ImagePacket));
    ph.type=GetTexture;
    imagepckt->id=id;
    imagepckt->header=ph;

    int size=Packet_serialize(buf_send,&(imagepckt->header));
    if (size==-1) ERROR_HELPER(-1,"Packet serialize didn't worked on image packet");


    //Something is needed here

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending request for a vehicle texture");
        if (ret==0) break;
        bytes_sent+=ret;
		}

    free(&(imagepckt->header));

     while(1){
        ret= recv(socket, buf_recv,BUFFSIZE,0);
        if( ret==-1 && errno==EINTR){
            continue;
            ERROR_HELPER(ret,"Failed receive of texture vehicle elevation");
        }
        if(ret==0){
            break;
        }
        else break;
    }

    ImagePacket* received= (ImagePacket*)Packet_deserialize(buf_recv, bytes_read);

    return received->image;
}

void send_vehicle_texture(int socket, int id){
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];
    int ret;
    int bytes_read=0;
    int bytes_sent=0;

    PacketHeader ph;
    ImagePacket* imagepckt=(ImagePacket*)malloc(sizeof(ImagePacket));
    ph.type=PostTexture;
    imagepckt->id=id;
    imagepckt->header=ph;

    int size=Packet_serialize(buf_send, &(imagepckt->header));
    if (size==-1) ERROR_HELPER(-1,"Packet serialize didn't worked on image packet");

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending of vehicle texture");
        if (ret==0) break;
        bytes_sent+=ret;
    }

    Packet_free(&(imagepckt->header));
}


void send_goodbye(int socket, int id){
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];
    int ret;
    int bytes_read=0;
    int bytes_sent=0;

    PacketHeader ph;
    ph.type=PostDisconnect;
    IdPacket* idpckt=(IdPacket*)malloc(sizeof(IdPacket));
    idpckt->id=id;
    idpckt->header=ph;

    int size=Packet_serialize(buf_send, &(idpckt->header));
    if (size==-1) ERROR_HELPER(-1,"Packet serialize didn't worked on image packet");

    while(bytes_sent<size){
        ret=send(socket,buf_send+bytes_sent,size-bytes_sent,0);
        if (ret==-1 && errno==EINTR) continue;
        ERROR_HELPER(ret,"Failed sending of vehicle texture");
        if (ret==0) break;
        bytes_sent+=ret;
    }
    Packet_free(&(idpckt->header));
}





