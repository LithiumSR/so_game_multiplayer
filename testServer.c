
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
#define SERVER_PORT     3000
int id;
void session(int sock_fd) {
    int ret;
    char buf_id[3000];
    printf("allocate an IDPacket\n");
    IdPacket* id_packet = (IdPacket*)malloc(sizeof(IdPacket));
    PacketHeader id_head;
    id_head.type = GetId;

    id_packet->header = id_head;
    id_packet->id = id++;
    
    //RICEVO ID
    
  printf("build packet with:\ntype\t%d\nsize\t%d\nid\t%d\n",
      id_packet->header.type,
      id_packet->header.size,
      id_packet->id);
    int msg_len=Packet_serialize(buf_id,&(id_packet->header));
    printf("Sto per inviare un pacchetto con %d bytes \n",msg_len);
    int bytes_sent=0;
    while(bytes_sent<msg_len){
			ret=send(sock_fd,buf_id+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
    }
    printf("[SESSION] IdPacket inviato \n");
    bytes_sent=0;
    
    //Attendo ricerca Immagine 
    char buf_rcv[10000];
    msg_len=0;
    int buf_len=10000;
    while(1){
        msg_len=recv(sock_fd, buf_rcv, buf_len, 0);
        if (msg_len==-1 && errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Cannot read from socket");
        if(msg_len!=0) break;
        }
    
    printf("Ricevuta richiesta di %d bytes \n",msg_len);
    PacketHeader* pheader=Packet_deserialize(buf_rcv,buf_len);
    if(pheader->type==GetTexture){
        printf("HO RICEVUTA UNA GETTEXTURE \n");
        return;
    }
    //Invio map texture
    char buf_img[1000000];
    Image* im;
    im = Image_load("./images/test.pgm");
    if(im!=NULL) printf("Image loaded \n");
    ImagePacket* image_packet = (ImagePacket*)malloc(sizeof(ImagePacket));
    PacketHeader im_head;
    im_head.type=PostTexture;
    image_packet->image=im;
    image_packet->header=im_head;
    msg_len= Packet_serialize(buf_img, &image_packet->header);
    printf("bytes written in the buffer: %d\n", msg_len);
    bytes_sent=0;
    while(bytes_sent<msg_len){
			ret=send(sock_fd,buf_img+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
    }
    printf("[SESSION] Image sent \n");
    

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
    while (1) {
        struct sockaddr_in client_addr = {0};

        // Setup to accept client connection
        int client_desc = accept(server_desc, (struct sockaddr*)&client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue;
        ERROR_HELPER(client_desc, "[MAIN THREAD] Impossibile eseguire accept() su server_desc");

        // Process request
        session(client_desc);

        // Close connection
        ret = close(client_desc);
        ERROR_HELPER(ret, "[MAIN THREAD] Impossibile chiudere la socket client_desc per la connessione accettata");

    }

}

