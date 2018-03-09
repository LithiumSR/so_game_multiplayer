#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include "common.h"
#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "client_op.h"
#include "server_op.h"
#include "so_game_protocol.h"
#include "linked_list.h"

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
int connectivity=1;
int checkUpdate=1;
int hasUsers=0;
ListHead* users;
long SERVER_PORT;
uint16_t  port_number_no;
typedef struct {
    int client_desc;
    Image* elevation_texture;
    Image* vehicle_texture;
    Image* surface_texture;
} tcp_args;

typedef struct {
    tcp_args args;
    int id_client; //I will use the socket as ID
} auth_args;

void handle_signal(int signal){
    // Find out which signal we're handling
    switch (signal) {
        case SIGHUP:
            break;
        case SIGINT:
            connectivity=0;
            checkUpdate=0;
            break;
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }
}

int UDP_Handler(char* buf_rcv,struct sockaddr_in client_addr){
    PacketHeader* ph=(PacketHeader*)buf_rcv;
    switch(ph->type){
        case(VehicleUpdate):{
            VehicleUpdatePacket* vup=(VehicleUpdatePacket*)Packet_deserialize(buf_rcv, ph->size);
            pthread_mutex_lock(&mutex);
            debug_print("[UDP_Handler] Received VehicleUpdatePacket from id  %d \n",vup->id);
            ListItem* client = List_find_by_id(users, vup->id);
            if(client == NULL) {
                debug_print("[UDP_Handler] Can't find the user to apply the update \n");
                Packet_free(&vup->header);
                pthread_mutex_unlock(&mutex);
                return -1;
            }
            client->x=vup->x;
            client->y=vup->y;
            client->theta=vup->theta;
            client->user_addr=client_addr;
            client->isAddrReady=1;
            client->current_time=vup->time;
            pthread_mutex_unlock(&mutex);
            debug_print("[UDP_Handler] VehicleUpdatePacket applied \n");
            Packet_free(&vup->header);
            return 0;
        }
        default: return -1;

    }
}

int TCP_Handler(int socket_desc,char* buf_rcv,Image* texture_map,Image* elevation_map,int id,int* isActive){
    PacketHeader* header=(PacketHeader*)buf_rcv;
    if(header->type==GetId){
        char buf_send[BUFFERSIZE];
        IdPacket* response=(IdPacket*)malloc(sizeof(IdPacket));
        PacketHeader ph;
        ph.type=GetId;
        response->header=ph;
        response->id=id;
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
        Packet_free(&(response->header));
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
        free(image_packet);
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
        int bytes_sent=0;
        int ret=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
            }
        free(image_packet);
        printf("Inviati %d bytes \n",msg_len);
        return 0;
    }
    else if(header->type==PostTexture){
        ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, header->size);
        Image* user_texture=deserialized_packet->image;

        pthread_mutex_lock(&mutex);
        ListItem* user= List_find_by_id(users,id);
        if (user==NULL){
            debug_print("[TCP Handler] USER NOT FOUND \n");
            return -1;
        }
        user->v_texture=user_texture;
        pthread_mutex_unlock(&mutex);
        debug_print("Texture veicolo aggiunta alla lista \n");
        free(deserialized_packet);
        return 0;
    }
    else if(header->type==PostDisconnect){
        printf("Trovato tentativo disconnessione \n");
        fflush(stdout);
        *isActive=0;
        return 0;
    }

    else {
        *isActive=0;
        printf("[TCP Handler] Pacchetto non riconosciuto \n");
        return -1;
    }
}

void* tcp_flow(void* args){
    tcp_args* arg=(tcp_args*)args;
    int sock_fd=arg->client_desc;
    pthread_mutex_lock(&mutex);
    ListItem* user=malloc(sizeof(ListItem));
    user->v_texture = NULL;
    user->creation_time=time(NULL);
    user->id=sock_fd;
    debug_print("Assegnato ID %d \n",sock_fd);
    List_insert(users, 0, user);
    hasUsers=1;
    pthread_mutex_unlock(&mutex);

    int ph_len=sizeof(PacketHeader);
    int isActive=1;
    int count=0;
    while (connectivity && isActive){
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
        //printf("Letti %d bytes da socket TCP \n",msg_len+ph_len);
        int ret=TCP_Handler(sock_fd,buf_rcv,arg->surface_texture,arg->elevation_texture,arg->client_desc,&isActive);
        ERROR_HELPER(ret,"TCP Handler failed");
        count++;
    }
    debug_print("Removing disconnected user....");
    pthread_mutex_lock(&mutex);
    ListItem* el=List_detach(users,user);
    if(el==NULL) goto END;
    Image* user_texture=el->v_texture;
    if(user_texture!=NULL) Image_free(user_texture);
    free(el);
    END:pthread_mutex_unlock(&mutex);
    debug_print("Done \n");
    pthread_exit(NULL);
}

void* udp_receiver(void* args){
    int socket_udp=*(int*)args;
    while(connectivity){
        if(!hasUsers){
            sleep(1);
            continue;
        }
        debug_print("[UDP_Receiver] Waiting for an VehicleUpdatePacket over UDP \n");
        char buf_recv[BUFFERSIZE];
        struct sockaddr_in client_addr = {0};
        socklen_t addrlen= sizeof(struct sockaddr_in);
        int ret=recvfrom(socket_udp,buf_recv,BUFFERSIZE,0, (struct sockaddr*)&client_addr,&addrlen);
        debug_print("[UDP] Letti %d bytes \n",ret);
        ERROR_HELPER(ret,"Errore");
        if(ret==-1)  goto END;
		if(ret == 0) continue;
		ret = UDP_Handler(buf_recv,client_addr);
        END: sleep(1);
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    int ret=0;
    if (argc<4) {
        debug_print("usage: %s <elevation_image> <texture_image> <port_number>\n", argv[1]);
        exit(-1);
    }
    char* elevation_filename=argv[1];
    char* texture_filename=argv[2];
    char* vehicle_texture_filename="./images/arrow-right.ppm";
    SERVER_PORT = strtol(argv[3], NULL, 0);
    if (SERVER_PORT < 1024 || SERVER_PORT > 49151) {
        fprintf(stderr, "Use a port number between 1024 and 49151.\n");
        exit(EXIT_FAILURE);
        }

    // load the images
    debug_print("[Main] loading elevation image from %s ... ", elevation_filename);
    Image* surface_elevation = Image_load(elevation_filename);
    if (surface_elevation) {
        debug_print("Done! \n");
        }
    else {
        debug_print("Fail! \n");
    }
    debug_print("[Main] loading texture image from %s ... ", texture_filename);
    Image* surface_texture = Image_load(texture_filename);
    if (surface_texture) {
        debug_print("Done! \n");
    }
    else {
        debug_print("Fail! \n");
    }

    debug_print("[Main] loading vehicle texture (default) from %s ... ", vehicle_texture_filename);
    Image* vehicle_texture = Image_load(vehicle_texture_filename);
    if (vehicle_texture) {
        debug_print("Done! \n");
    }
    else {
        debug_print("Fail! \n");
    }

    port_number_no = htons((uint16_t)SERVER_PORT);

    // setup tcp socket
    debug_print("[MAIN] Starting TCP socket \n");

    int server_tcp = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(server_tcp, "[MAIN] Impossibile creare socket server_tcp");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;

    int reuseaddr_opt = 1; // recover server if a crash occurs
    ret = setsockopt(server_tcp, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile settare l'opzione SO_REUSEADDR per socket server_desc");

    int sockaddr_len = sizeof(struct sockaddr_in);
    ret = bind(server_tcp, (struct sockaddr*) &server_addr, sockaddr_len); // binding dell'indirizzo
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire bind() su server_desc");

    ret = listen(server_tcp, 16); // flag socket as passive
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire listen() su server_desc");

    debug_print("[MAIN] TCP socket successfully created \n");

    //init List structure
    users = malloc(sizeof(ListHead));
	List_init(users);
    debug_print("[MAIN] Initialized users list \n");

    //seting signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigfillset(&sa.sa_mask);
    ret=sigaction(SIGHUP, &sa, NULL);
    ERROR_HELPER(ret,"Error: cannot handle SIGHUP");
    //ret=sigaction(SIGINT, &sa, NULL);
    //ERROR_HELPER(ret,"Error: cannot handle SIGINT");

    debug_print("[MAIN] Custom signal handlers are now enabled \n");
    //preparing 2 threads (1 for udp socket, 1 for tcp socket)

    //Creating UDP Socket

    uint16_t port_number_no_udp= htons((uint16_t)UDPPORT);

    // setup server
    int server_udp = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(server_udp, "[MAIN THREAD] Impossibile creare socket server_desc");

    struct sockaddr_in udp_server = {0};
    udp_server.sin_addr.s_addr = INADDR_ANY;
    udp_server.sin_family      = AF_INET;
    udp_server.sin_port        = port_number_no_udp;

    int reuseaddr_opt_udp = 1; // recover server if a crash occurs
    ret = setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt_udp, sizeof(reuseaddr_opt_udp));
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile settare l'opzione SO_REUSEADDR per socket server_desc");

    ret = bind(server_udp, (struct sockaddr*) &udp_server, sizeof(udp_server)); // binding dell'indirizzo
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire bind() su server_desc");

    debug_print("UDP socket created \n");
    pthread_t UDP_setup;
    ret = pthread_create(&UDP_setup, NULL,udp_receiver, &server_udp);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread tcp failed");

    while (connectivity) {
        struct sockaddr_in client_addr = {0};
        // Setup to accept client connection
        int client_desc = accept(server_tcp, (struct sockaddr*)&client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue;
        ERROR_HELPER(client_desc, "[MAIN THREAD] Impossibile eseguire accept() su server_desc");

        tcp_args tcpArgs;

        tcpArgs.client_desc=client_desc;
        tcpArgs.elevation_texture = surface_elevation;
        tcpArgs.surface_texture = surface_texture;
        tcpArgs.vehicle_texture = vehicle_texture;

        pthread_t threadTCP;
        ret = pthread_create(&threadTCP, NULL,tcp_flow, &tcpArgs);
        PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread tcp failed");
        ret = pthread_detach(threadTCP);
    }

    ret=pthread_join(UDP_setup,NULL);
    ERROR_HELPER(ret,"Something went wrong with the join on UDP_setup thread");
    debug_print("[Main] Closing the server...");
    List_destroy(users);
    close(server_tcp);

    Image_free(surface_elevation);
	Image_free(surface_texture);
	Image_free(vehicle_texture);

    exit(EXIT_SUCCESS);
}
