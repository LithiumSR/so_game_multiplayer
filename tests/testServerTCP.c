
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <errno.h>
#include "../common/common.h"
#include <pthread.h>
#include "../game_framework/so_game_protocol.h"
#define BUFFERSIZE 1000000
int id;

typedef struct {
    int server_tcp;
    Image* elevation_map;
    Image* texture_map;
} tcp_args;


int TCP_Handler(int socket_desc,char* buf_rcv,Image* texture_map,Image* elevation_map){
    PacketHeader* header=(PacketHeader*)buf_rcv;
    if(header->type==GetId){
        char buf_send[BUFFERSIZE];
        IdPacket* response=(IdPacket*)malloc(sizeof(IdPacket));
        PacketHeader ph;
        ph.type=GetId;
        response->header=ph;
        response->id=id++;
        int msg_len=Packet_serialize(buf_send,&(response->header));
        printf("[Get ID] bytes written in the buffer: %d\n", msg_len);
        int ret=0;
        int bytes_sent=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
        }

        printf("Inviati %d bytes \n",bytes_sent);
        return 0;
    }
    else if(header->type==GetTexture){
        char buf_send[BUFFERSIZE];
        ImagePacket* image_packet = (ImagePacket*)malloc(sizeof(ImagePacket));
        PacketHeader im_head;
        im_head.type=PostTexture;
        image_packet->image=texture_map;
        image_packet->header=im_head;
        int msg_len= Packet_serialize(buf_send, &image_packet->header);
        printf("[Map Texture] bytes written in the buffer: %d\n", msg_len);
        int bytes_sent=0;
        int ret=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
            if (ret==-1 && errno==ECONNRESET) return -1;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
            }
        printf("Inviati %d bytes \n",bytes_sent);
        return 0;
    }

    else if(header->type==GetElevation){
        char buf_send[BUFFERSIZE];
        ImagePacket* image_packet = (ImagePacket*)malloc(sizeof(ImagePacket));
        PacketHeader im_head;
        im_head.type=PostElevation;
        image_packet->image=elevation_map;
        image_packet->header=im_head;
        int msg_len= Packet_serialize(buf_send, &image_packet->header);
        printf("[Map Elevation] bytes written in the buffer: %d\n", msg_len);
        buf_send[msg_len]='\n';
        int bytes_sent=0;
        int ret=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
            }
        printf("Inviati %d bytes \n",msg_len);
        return 0;
    }
    else if(header->type==PostTexture){
        if(id==-1) return -1;
        ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, header->size);
        Image* user_texture=deserialized_packet->image;
        printf("Texture veicolo deserializzato \n");
        return 0;
    }
    else if(header->type==PostDisconnect){
        printf("Trovato tentativo disconnessione");
        fflush(stdout);
        return 0;
    }

    else {
        printf("Pacchetto non riconosciuto \n");
        return -1;
    }
}

void* session(void* args) {
    tcp_args* arg=(tcp_args*)args;
    int sock_fd=arg->server_tcp;
    int ph_len=sizeof(PacketHeader);

    int count=0;
    while (1){
        int msg_len=0;
        char buf_rcv[BUFFERSIZE];
        while(msg_len<ph_len){
            int ret=recv(sock_fd, buf_rcv+msg_len, ph_len-msg_len, 0);
            if (ret==-1 && errno == EINTR) continue;
            ERROR_HELPER(msg_len, "Cannot read from socket");
            msg_len+=ret;
            }
        PacketHeader* header=(PacketHeader*)buf_rcv;
        int size_remaining=header->size-ph_len;
        msg_len=0;
        while(msg_len<size_remaining){
            int ret=recv(sock_fd, buf_rcv+msg_len+ph_len, size_remaining-msg_len, 0);
            if (ret==-1 && errno == EINTR) continue;
            ERROR_HELPER(msg_len, "Cannot read from socket");
            msg_len+=ret;
        }
        printf("Letti %d bytes da socket TCP",msg_len+ph_len);
        int ret=TCP_Handler(sock_fd,buf_rcv,arg->texture_map,arg->elevation_map);
        ERROR_HELPER(ret,"TCP Handler failed");
        count++;
    }
    pthread_exit(NULL);
}


int main(int argc, char **argv) {

 if (argc != 2) {
       fprintf(stderr, "Sintassi: %s <port_number>\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    int ret;
    id=0;
    // Get port number
    uint16_t port_number_no;
    long tmp = strtol(argv[1], NULL, 0); // convert a string to long
    if (tmp < 1024 || tmp > 49151) {
        fprintf(stderr, "Utilizzare un numero di porta compreso tra 1024 e 49151.\n");
        exit(EXIT_FAILURE);
    }
    port_number_no = htons((uint16_t)tmp);

    // setup server
    int server_desc = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(server_desc, "[MAIN THREAD] Impossibile creare socket server_desc");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;

    int reuseaddr_opt = 1; // recover server if a crash occurs
    ret = setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile settare l'opzione SO_REUSEADDR per socket server_desc");

    int sockaddr_len = sizeof(struct sockaddr_in);
    ret = bind(server_desc, (struct sockaddr*) &server_addr, sockaddr_len); // binding dell'indirizzo
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire bind() su server_desc");

    ret = listen(server_desc, 16); // flag socket as passive
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire listen() su server_desc");
    Image* texture_map=Image_load("./images/test.pgm");
    if (texture_map==NULL) ERROR_HELPER(-1,"Errore caricamento texture map");
    Image* elevation_map=Image_load("./images/test.ppm");
    if (elevation_map==NULL) ERROR_HELPER(-1,"Errore caricamento elevation map");


    while (1) {
        struct sockaddr_in client_addr = {0};
        // Setup to accept client connection
        int client_desc = accept(server_desc, (struct sockaddr*)&client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue;
        ERROR_HELPER(client_desc, "[MAIN THREAD] Impossibile eseguire accept() su server_desc");

        tcp_args tcpArgs;
        tcpArgs.server_tcp=client_desc;
        tcpArgs.elevation_map=elevation_map;
        tcpArgs.texture_map=texture_map;
        pthread_t threadTCP;
        ret = pthread_create(&threadTCP, NULL,session, &tcpArgs);
        PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread tcp failed");
        ret = pthread_detach(threadTCP);

    }
    printf("Esco");

}

