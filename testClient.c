
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
#define SERVER_PORT     2015

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
    int msg_len=0;
    char buf[1000000];
    int buf_len=1000000;
    while(1){
        msg_len=recv(socket_desc, buf, buf_len, 0);
        if (msg_len==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        if(msg_len!=0) break;
        }
    
    printf("Ho ricevuto %d bytes \n",msg_len);
    IdPacket* deserialized_packet = (IdPacket*)Packet_deserialize(buf, msg_len);
    printf("Ricevuto id %d \n",deserialized_packet->id);
    int id=deserialized_packet->id;
    
    
    ImagePacket* imgpck=(ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader ph;
    ph.type=GetTexture;
    imgpck->id=id;
    imgpck->header=ph;
    
    char buf_send[1000000];
    msg_len=Packet_serialize(buf_send,&(imgpck->header));
    printf("Devo inviare %d bytes con la richiesta della map texture \n",msg_len);
    int bytes_sent=0;
    while(bytes_sent<msg_len){
			ret=send(socket_desc,buf+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
    }
    printf("Inviati %d bytes_sent con la richiesta per la map texture \n",bytes_sent);
    
    msg_len=0;
    
    while(1){
        msg_len=recv(socket_desc, buf, buf_len, 0);
        if (msg_len==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        if(msg_len!=0) break;
        }
    printf("Ricevuti %d bytes di texture\n", msg_len);

}

