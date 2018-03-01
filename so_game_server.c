
// #include <GL/glut.h> // not needed here
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
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
ListHead* users;

typedef struct {
    int server_tcp;
    Image* e_texture;
    Image* v_texture;
    Image* s_texture;
} tcp_args;

typedef struct {
    tcp_args args;
    int id_client; //I will use the socket as ID
} auth_args;

void handle_signal(int signal){
    sigset_t pending;
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

int UDP_Packet_handler(char* buf_rcv,struct sockaddr_in client_addr){
    PacketHeader* ph=(PacketHeader*)buf_rcv;
    switch(ph->type){
        case(VehicleUpdate):{
            int flag=0;
            VehicleUpdatePacket* vup=(VehicleUpdatePacket*)Packet_deserialize(buf_rcv, ph->size);
            pthread_mutex_lock(&mutex);
            ListItem* client = List_find_by_id(users, vup->id);
            if(client == NULL) {
                flag=1;
                goto END;
            }
            client->x=vup->x;
            client->y=vup->y;
            client->theta=vup->theta;
            client->user_addr=client_addr;
            client->isAddrReady=1;
            pthread_mutex_unlock(&mutex);
            END: Packet_free(&vup->header);
            if(flag) return -1; else return 0;
        }
        default: return -1;

    }
}

int TCP_Packet_handler(char* buf_rcv, char* buf_send, int socket, Image* elevation_map, Image* surface_texture, Image* vehicle_texture, int* isActive){
    PacketHeader* ph_rcv = (PacketHeader*)buf_rcv;
    switch(ph_rcv->type){
        case(GetId): {
            printf("Found getID \n");
            IdPacket* idpckt = (IdPacket*)Packet_deserialize(buf_rcv, ph_rcv->size);
            if(idpckt->id != -1){
                Packet_free(&idpckt->header);
                return -1;
            }
            IdPacket* response = malloc(sizeof(IdPacket));
            PacketHeader ph;
            ph.type=GetId;
            response->header=ph;
            response->id=socket;
            int size = Packet_serialize(buf_send, &response->header);
            Packet_free(&response->header);
            Packet_free(&idpckt->header);
            return size;
        }

        case(GetElevation): {
            printf("Found getElevation \n");
            ImagePacket* imgpckt = (ImagePacket*)Packet_deserialize(buf_rcv, ph_rcv->size);
            if(imgpckt->id == socket){
                ImagePacket* response = malloc(sizeof(ImagePacket));
                PacketHeader ph;
                ph.type=PostTexture;
                response->id=socket;
                response->image=elevation_map;
                response->header=ph;
                int size=Packet_serialize(buf_send, &response->header);
                Packet_free(&response->header);
                Packet_free(&imgpckt->header);
                return size;
            }
            else {
                Packet_free(&imgpckt->header);
                return -1;
            }
            break;
        }

        case(GetTexture): {
            printf("Found getTexture \n");
            ImagePacket* imgpckt = (ImagePacket*)Packet_deserialize(buf_rcv, ph_rcv->size);
            if(imgpckt->id == socket){
                ImagePacket* response = (ImagePacket*) malloc(sizeof(ImagePacket));
                PacketHeader ph;
                ph.type=PostTexture;
                response->id=socket;
                response->image=surface_texture;
                response->header=ph;
                int size=Packet_serialize(buf_send, &response->header);
                Packet_free(&response->header);
                Packet_free(&imgpckt->header);
                return size;
            }
            else {
                Packet_free(&imgpckt->header);
                return -1;
            }
            break;
        }

        case(PostTexture): {
            ImagePacket* imgpckt = (ImagePacket*)Packet_deserialize(buf_rcv, ph_rcv->size);
            if (imgpckt->id <0) {
                Packet_free(&imgpckt->header);
                return -1;
            }
            Image* img = imgpckt->image;

            pthread_mutex_lock(&mutex);
            ListItem* user= List_find_by_id(users,socket);
            user->v_texture=img;
            pthread_mutex_unlock(&mutex);

            return 0;
        }

        case(PostDisconnect):{
            *isActive=0;
            return 0;
        }
        default: return -1;
    }
    }


void* auth_routine(void* args){
    auth_args authArgs = *(auth_args*)args;
    int client_sock=authArgs.id_client;
    char buf_rcv[BUFFSIZE];
    char buf_send[BUFFSIZE];

    memset((void*)buf_rcv,'\0',BUFFSIZE);
    memset((void*)buf_send,'\0',BUFFSIZE);

    pthread_mutex_lock(&mutex);
    ListItem* user=malloc(sizeof(ListItem));
    user->v_texture = authArgs.args.v_texture;
    List_insert(users, 0, user);
    pthread_mutex_unlock(&mutex);
    int isActive=1;
    while(connectivity && isActive){
        debug_print("Waiting for a data packet \n");
        int bytes_read = recv(client_sock,buf_rcv, BUFFSIZE, 0);
        if (bytes_read==0) continue;
        else if (bytes_read<0) break;
        int size = TCP_Packet_handler(buf_rcv, buf_send,client_sock, authArgs.args.e_texture,authArgs.args.s_texture,authArgs.args.v_texture,&isActive);
        if(size <= 0 ) continue;
        debug_print("Received %d bytes \n. Sending response... \n",size);
        int bytes_sent=0;
        int ret=0;
        while(bytes_sent<size){
            ret=send(client_sock,buf_send+bytes_sent,size-bytes_sent,0);
            if (ret==-1 && errno==EINTR) continue;
            ERROR_HELPER(ret,"Errore invio \n");
            if (ret==0) break;
            bytes_sent+=ret;
		}

        debug_print("Response sent. \n");
        memset((void*)buf_rcv,'\0',BUFFSIZE);
        memset((void*)buf_send,'\0',BUFFSIZE);

    }

    pthread_mutex_lock(&mutex);
    ListItem* el=List_detach(users,user);
    pthread_mutex_unlock(&mutex);
    free(el);
    pthread_exit(NULL);
}

void* tcp_flow(void* args){
    tcp_args tcpArgs = *(tcp_args*)args;
    int server_tcp=tcpArgs.server_tcp;
    debug_print("Waiting for a client");
    while(connectivity==1){
        int addrlen = sizeof(struct sockaddr_in);
		struct sockaddr_in client;

        int client_sock = accept(server_tcp, (struct sockaddr*) &client, (socklen_t*) &addrlen);
        if (client_sock==-1) continue;
        pthread_t auth_thread;
        auth_args argAuth;
        argAuth.args=tcpArgs;
        argAuth.id_client=client_sock;
        int ret = pthread_create(&auth_thread, NULL, auth_routine,&argAuth);
        PTHREAD_ERROR_HELPER(ret, "[TCP Flow Thread] Error when creating Auth thread");
		ret = pthread_detach(auth_thread);
		PTHREAD_ERROR_HELPER(ret, "[TCP Flow Thread] Errore detach thread");
    }

    pthread_exit(NULL);
}



void* udp_send(void* args){
    int socket_udp=*(int*)args;
    char buf_send[BUFFSIZE];
    char buf_recv[BUFFSIZE];
    while(connectivity && checkUpdate){
            PacketHeader ph;
            ph.type=WorldUpdate;
            WorldUpdatePacket* wup=(WorldUpdatePacket*)malloc(sizeof(WorldUpdatePacket));
            wup->header=ph;
            wup->num_vehicles=users->size;
            wup->updates=(ClientUpdate*)malloc(sizeof(ClientUpdate)*wup->num_vehicles);
            pthread_mutex_lock(&mutex);
            ListItem* client= users->first;
            int i=0;
            while(i<(wup->num_vehicles)){
                ClientUpdate* cup= &(wup->updates[i]);
                cup->y=client->y;
                cup->x=client->x;
                cup->theta=client->theta;
                cup->id=client->id;
                if(i<=wup->num_vehicles-1) client = client->next;
                i++;
            }
            pthread_mutex_unlock(&mutex);
            int size=Packet_serialize(buf_recv,&wup->header);

            if(size==0 || size==-1){
                sleep(1000);
				continue;
			}

            pthread_mutex_lock(&mutex);
            i=0;
            client=users->first;
            while(i<(wup->num_vehicles)){
                if(client->isAddrReady==1){
                    int ret = sendto(socket_udp, buf_send, size, 0, (struct sockaddr*) &client->user_addr, (socklen_t) sizeof(client->user_addr));
                    debug_print("Inviato update di %d bytes",ret);
                }
                if(i!=wup->num_vehicles - 1) client = client->next;
                i++;
            }
            pthread_mutex_unlock(&mutex);
        }

    pthread_exit(NULL);
}

void* udp_receive(void* args){
    int socket_udp=*(int*)args;
    char buf_recv[BUFFSIZE];
    struct sockaddr_in client_addr;
    while(connectivity){
        memset(&client_addr, 0, sizeof(client_addr));
        int ret=recvfrom(socket_udp, buf_recv, BUFFSIZE, 0, (struct sockaddr*) &client_addr, (socklen_t*) sizeof(client_addr));
        if(ret==-1) connectivity=0;
		if(ret == 0) continue;

		ret = UDP_Packet_handler(buf_recv,client_addr);
        sleep(1000);
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


  // load the images
debug_print("loading elevation image from %s ... ", elevation_filename);
  Image* surface_elevation = Image_load(elevation_filename);
  if (surface_elevation) {
    debug_print("Done! \n");
  } else {
    debug_print("Fail! \n");
  }


  debug_print("loading texture image from %s ... ", texture_filename);
  Image* surface_texture = Image_load(texture_filename);
  if (surface_texture) {
    debug_print("Done! \n");
  } else {
    debug_print("Fail! \n");
  }

  debug_print("loading vehicle texture (default) from %s ... ", vehicle_texture_filename);
  Image* vehicle_texture = Image_load(vehicle_texture_filename);
  if (vehicle_texture) {
    debug_print("Done! \n");
  } else {
    debug_print("Fail! \n");
  }


    uint16_t port_number_no;
    long tmp = strtol(argv[3], NULL, 0); // convert a string to long
    if (tmp < 1024 || tmp > 49151) {
        fprintf(stderr, "Utilizzare un numero di porta compreso tra 1024 e 49151.\n");
        exit(EXIT_FAILURE);
    }
    port_number_no = htons((uint16_t)tmp);

    // setup tcp socket
    fprintf(stdout,"[MAIN THREAD] Starting TCP socket \n");

    int server_tcp = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(server_tcp, "[MAIN THREAD] Impossibile creare socket server_tcp");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;

    int reuseaddr_opt = 1; // recover server if a crash occurs
    ret = setsockopt(server_tcp, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile settare l'opzione SO_REUSEADDR per socket server_tcp");
    ret = SetSocketBlockingEnabled(server_tcp,1);
	ERROR_HELPER(ret, "[MAIN THREAD] Impossibile rendere la socket non bloccante con  SetSocketBlockingEnabled()");

    int sockaddr_len = sizeof(struct sockaddr_in);
    ret = bind(server_tcp, (struct sockaddr*) &server_addr, sockaddr_len); // binding dell'indirizzo
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire bind() su server_tcp");

    ret = listen(server_tcp, 16); // flag socket as passive
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire listen() su server_tcp");


    //setup udp socket
    int server_udp = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(server_udp, "[MAIN THREAD] Impossibile creare socket server_udp");
    ret = bind(server_udp, (struct sockaddr*) &server_addr, sockaddr_len);
    ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire bind() su server_udp");


    //init List structure
    users = malloc(sizeof(ListHead));
	List_init(users);

    //seting signal handlers
    struct sigaction sa;
    sa.sa_handler = &handle_signal;
    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;
    // Block every signal during the handler
    sigfillset(&sa.sa_mask);
    ret=sigaction(SIGHUP, &sa, NULL);
    ERROR_HELPER("Error: cannot handle SIGHUP");
    ret=sigaction(SIGINT, &sa, NULL);
    ERROR_HELPER("Error: cannot handle SIGINT");

    //preparing 2 threads (1 for udp socket, 1 for tcp socket)
    tcp_args tcpArgs;

    tcpArgs.server_tcp = server_tcp;
    tcpArgs.e_texture = surface_elevation;
    tcpArgs.s_texture = surface_texture;
    tcpArgs.v_texture = vehicle_texture;

    pthread_t threadTCP,threadUDPReceive,threadUDPSend;

    ret = pthread_create(&threadTCP, NULL,tcp_flow, &tcpArgs);
    PTHREAD_ERROR_HELPER(ret, "[MAIN THREAD] Creazione thread tcp fallita");

    ret = pthread_create(&threadUDPReceive, NULL, udp_receive, (void*) &server_udp);
    PTHREAD_ERROR_HELPER(ret, "[MAIN THREAD] Creazione thread udp fallita");

    ret = pthread_create(&threadUDPSend, NULL, udp_send, (void*) &server_udp);
    PTHREAD_ERROR_HELPER(ret, "[MAIN THREAD] Creazione thread udp fallita");

    ret= pthread_join(threadTCP,NULL);
    PTHREAD_ERROR_HELPER(ret,"[MAIN THREAD] Failed threadTCP join");
    ret= pthread_join(threadUDPReceive,NULL);
    PTHREAD_ERROR_HELPER(ret,"[MAIN THREAD] Failed threadUDPReceive join");
    ret= pthread_join(threadUDPSend,NULL);
    PTHREAD_ERROR_HELPER(ret,"[MAIN THREAD] Failed threadUDPSend join");

    //WIP  SERVER CLEANUP
    debug_print("Closing the server...");
    List_destroy(users);
    close(server_tcp);
	close(server_udp);

    Image_free(surface_elevation);
	Image_free(surface_texture);
	Image_free(vehicle_texture);

    exit(EXIT_SUCCESS);
}
