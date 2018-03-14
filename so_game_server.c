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
#include "so_game_protocol.h"
#include "client_list.h"

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
int connectivity=1;
int exchangeUpdate=1;
int cleanGarbage=1;
int hasUsers=0;
ClientListHead* users;
uint16_t  port_number_no;
int server_tcp=-1;
int server_udp;
World serverWorld;
typedef struct {
    int client_desc;
    Image* elevation_texture;
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
            exchangeUpdate=0;
            cleanGarbage=0;
            shutdown(server_tcp, SHUT_RDWR);
            shutdown(server_udp, SHUT_RDWR);
            exit(0);
            break;
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }
}

//Send a postDisconnect packet to a client over UDP
void sendDisconnect(int socket_udp, struct sockaddr_in client_addr){
    char buf_send[BUFFERSIZE];
    PacketHeader ph;
    ph.type=PostDisconnect;
    IdPacket* ip=(IdPacket*)malloc(sizeof(IdPacket));
    ip->id=-1;
    ip->header=ph;
    int size=Packet_serialize(buf_send,&(ip->header));
    int ret=sendto(socket_udp, buf_send, size, 0, (struct sockaddr*) &client_addr, (socklen_t) sizeof(client_addr));
    Packet_free(&(ip->header));
    debug_print("[UDP_Receiver] Sent PostDisconnect packet of %d bytes to unrecognized user \n",ret);
}


int UDP_Handler(int socket_udp,char* buf_rcv,struct sockaddr_in client_addr){
    PacketHeader* ph=(PacketHeader*)buf_rcv;
    switch(ph->type){
        case(VehicleUpdate):{
            VehicleUpdatePacket* vup=(VehicleUpdatePacket*)Packet_deserialize(buf_rcv, ph->size);
            pthread_mutex_lock(&mutex);
            ClientListItem* client = ClientList_find_by_id(users, vup->id);
            if(client == NULL) {
                debug_print("[UDP_Handler] Can't find the user with id %d to apply the update \n",vup->id);
                Packet_free(&vup->header);
                sendDisconnect(socket_udp,client_addr);
                pthread_mutex_unlock(&mutex);
                return -1;
            }

            if(!client->insideWorld){
                debug_print("[Info] Skipping update of a vehicle that isn't inside the world simulation \n");
                pthread_mutex_unlock(&mutex);
                return 0;
            }

            setForcesUpdate(client->vehicle,vup->translational_force,vup->rotational_force);
            client->user_addr=client_addr;
            client->isAddrReady=1;
            client->last_update_time=vup->time;
            pthread_mutex_unlock(&mutex);
            fprintf(stdout,"[UDP_Receiver] Applied VehicleUpdatePacket with force_translational_update: %f force_rotation_update: %f.. \n",vup->translational_force,vup->rotational_force);
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
        debug_print("[Send ID] bytes written in the buffer: %d\n", msg_len);
        int ret=0;
        int bytes_sent=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Can't assign ID");
			if (ret==0) break;
			bytes_sent+=ret;
        }
        Packet_free(&(response->header));
        debug_print("[Send ID] Sent %d bytes \n",bytes_sent);
        return 0;
    }
    else if(header->type==GetTexture){
        char buf_send[BUFFERSIZE];
        ImagePacket* image_request = (ImagePacket*)buf_rcv;
        if(image_request->id>=0){
            if(image_request->id==0) debug_print("[WARNING] Received GetTexture with id 0 which is highly unlikeable \n");
            char buf_send[BUFFERSIZE];
            ImagePacket* image_packet = (ImagePacket*)malloc(sizeof(ImagePacket));
            PacketHeader im_head;
            im_head.type=PostTexture;
            pthread_mutex_lock(&mutex);
            ClientListItem* el=ClientList_find_by_id(users,image_request->id);
            if (el==NULL) {
                pthread_mutex_unlock(&mutex);
                return -1;
            }
            image_packet->image=el->v_texture;
            pthread_mutex_unlock(&mutex);
            image_packet->header=im_head;
            int msg_len= Packet_serialize(buf_send, &image_packet->header);
            debug_print("[Send Vehicle Texture] bytes written in the buffer: %d\n", msg_len);
            int bytes_sent=0;
            int ret=0;
            while(bytes_sent<msg_len){
                ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
                if (ret==-1 && errno==EINTR) continue;
                ERROR_HELPER(ret,"Can't send map texture over TCP");
                bytes_sent+=ret;
            }

            free(image_packet);
            debug_print("[Send Vehicle Texture] Sent %d bytes \n",bytes_sent);
            return 0;
        }
        ImagePacket* image_packet =(ImagePacket*)malloc(sizeof(ImagePacket));
        PacketHeader im_head;
        im_head.type=PostTexture;
        image_packet->image=texture_map;
        image_packet->header=im_head;
        int msg_len= Packet_serialize(buf_send, &image_packet->header);
        debug_print("[Send Map Texture] bytes written in the buffer: %d\n", msg_len);
        int bytes_sent=0;
        int ret=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Can't send map texture over TCP");
			if (ret==0) break;
			bytes_sent+=ret;
            }
        free(image_packet);
        debug_print("[Send Map Texture] Sent %d bytes \n",bytes_sent);
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
        printf("[Send Map Elevation] bytes written in the buffer: %d\n", msg_len);
        int bytes_sent=0;
        int ret=0;
        while(bytes_sent<msg_len){
			ret=send(socket_desc,buf_send+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Can't send map elevation over TCP");
			if (ret==0) break;
			bytes_sent+=ret;
            }
        free(image_packet);
        debug_print("[Send Map Elevation] Sent %d bytes \n",bytes_sent);
        return 0;
    }
    else if(header->type==PostTexture){
        ImagePacket* deserialized_packet = (ImagePacket*)Packet_deserialize(buf_rcv, header->size);
        Image* user_texture=deserialized_packet->image;

        pthread_mutex_lock(&mutex);
        ClientListItem* user= ClientList_find_by_id(users,deserialized_packet->id);
        ClientList_print(users);
        fflush(stdout);

        if (user==NULL){
            debug_print("[Set Texture] User not found \n");
            pthread_mutex_unlock(&mutex);
             Packet_free(&(deserialized_packet->header));
            return -1;
        }
        if (user->insideWorld) {
            pthread_mutex_unlock(&mutex);
            Packet_free(&(deserialized_packet->header));
            return 0;
            }
        user->v_texture=user_texture;
        user->insideWorld=1;
        Vehicle* vehicle=(Vehicle*) malloc(sizeof(Vehicle));
        Vehicle_init(vehicle, &serverWorld, id, user->v_texture);
        user->vehicle=vehicle;
        World_addVehicle(&serverWorld, vehicle);
        debug_print("[Set Texture] AGGIUNTO VEICOLO con id %d",id);
        pthread_mutex_unlock(&mutex);
        debug_print("[Set Texture] Vehicle texture applied to user with id %d \n",id);
        free(deserialized_packet);
        return 0;
    }
    else if(header->type==PostDisconnect){
        debug_print("[Notify Disconnect] User disconnect...");
        *isActive=0;
        return 0;
    }

    else {
        *isActive=0;
        printf("[TCP Handler] Unknown packet. Cleaning resources...\n");
        return -1;
    }
}

//Handle authentication and disconnection
void* tcp_flow(void* args){
    tcp_args* arg=(tcp_args*)args;
    int sock_fd=arg->client_desc;
    pthread_mutex_lock(&mutex);
    ClientListItem* user=malloc(sizeof(ClientListItem));
    user->v_texture = NULL;
    user->creation_time=time(NULL);
    user->id=sock_fd;
    user->isAddrReady=0;
    user->forceRefresh=1;
    user->insideWorld=0;
    user->v_texture=NULL;
    user->vehicle=NULL;
    printf("[New user] Adding client with id %d \n",sock_fd);
    ClientList_insert(users,user);
    ClientList_print(users);
    hasUsers=1;
    pthread_mutex_unlock(&mutex);
    int ph_len=sizeof(PacketHeader);
    int isActive=1;
    while (connectivity && isActive){
        int msg_len=0;
        char buf_rcv[BUFFERSIZE];
        while(msg_len<ph_len){
            int ret=recv(sock_fd, buf_rcv+msg_len, ph_len-msg_len, 0);
            if (ret==-1 && errno == EINTR) continue;
            else if (ret<=0) goto EXIT;
            msg_len+=ret;
            }

        PacketHeader* header=(PacketHeader*)buf_rcv;
        int size_remaining=header->size-ph_len;
        msg_len=0;
        while(msg_len<size_remaining){
            int ret=recv(sock_fd, buf_rcv+msg_len+ph_len, size_remaining-msg_len, 0);
            if (ret==-1 && errno == EINTR) continue;
            else if(ret<=0) goto EXIT;
            msg_len+=ret;
            }
        int ret=TCP_Handler(sock_fd,buf_rcv,arg->surface_texture,arg->elevation_texture,arg->client_desc,&isActive);
        if (ret==-1) ClientList_print(users);
    }
    EXIT: printf("Freeing resources...");
    pthread_mutex_lock(&mutex);
    ClientListItem* el=ClientList_find_by_id(users,sock_fd);
    if(el==NULL) goto END;
    ClientListItem* del=ClientList_detach(users,el);
    if(del==NULL) goto END;
    if(!del->insideWorld) goto END;
    World_detachVehicle(&serverWorld,del->vehicle);
    Vehicle_destroy(del->vehicle);
    Image* user_texture=del->v_texture;
    if (user_texture!=NULL) Image_free(user_texture);
    if(users->size==0) hasUsers=0;
    free(del);
    ClientList_print(users);
    END: pthread_mutex_unlock(&mutex);
    close(sock_fd);
    pthread_exit(NULL);
}

//Receive and apply VehicleUpdatePacket from clients
void* udp_receiver(void* args){
    int socket_udp=*(int*)args;
    while(connectivity && exchangeUpdate){
        if(!hasUsers){
            sleep(1);
            continue;
        }

        char buf_recv[BUFFERSIZE];
        struct sockaddr_in client_addr = {0};
        socklen_t addrlen= sizeof(struct sockaddr_in);
        int bytes_read=recvfrom(socket_udp,buf_recv,BUFFERSIZE,0, (struct sockaddr*)&client_addr,&addrlen);
        if(bytes_read==-1)  goto END;
		if(bytes_read == 0) goto END;
        PacketHeader* ph=(PacketHeader*)buf_recv;
        if(ph->size!=bytes_read) {
            debug_print("[WARNING] Skipping partial UDP packet \n");
            goto END;
        }
		int ret = UDP_Handler(socket_udp,buf_recv,client_addr);
        if (ret==-1) debug_print("[UDP_Receiver] UDP Handler couldn't manage to apply the VehicleUpdate \n");
        END: usleep(50);
    }
    pthread_exit(NULL);
}

//Send WorldUpdatePacket to every client that sent al least one VehicleUpdatePacket
void* udp_sender(void* args){
    int socket_udp=*(int*)args;
    while(connectivity && exchangeUpdate){
        if(!hasUsers){
            sleep(1);
            continue;
        }
        char buf_send[BUFFERSIZE];

        PacketHeader ph;
        ph.type=WorldUpdate;
        WorldUpdatePacket* wup=(WorldUpdatePacket*)malloc(sizeof(WorldUpdatePacket));
        wup->header=ph;
        pthread_mutex_lock(&mutex);
        int n;
        ClientListItem* client= users->first;
        for(n=0;client!=NULL;client=client->next){
            if(client->isAddrReady) n++;
        }
        wup->num_vehicles=n;
        fprintf(stdout,"[UDP_Sender] Creating WorldUpdatePacket containing info about %d users \n",n);
        if(n==0){
            pthread_mutex_unlock(&mutex);
            sleep(1);
            continue;
        }
        World_update(&serverWorld);
        wup->updates=(ClientUpdate*)malloc(sizeof(ClientUpdate)*n);
        client= users->first;
        wup->time=time(NULL);
        for(int i=0;client!=NULL;i++){
            if(!(client->isAddrReady && client->insideWorld)) {
                client = client->next;
                continue;
            }
            ClientUpdate* cup= &(wup->updates[i]);
            if(client->forceRefresh==1) {
                cup->forceRefresh=1;
                client->forceRefresh=0;
            }
            else cup->forceRefresh=0;
            getXYTheta(client->vehicle,&(cup->x),&(cup->y),&(cup->theta));
            //getForces(client->vehicle,&(cup->rotational_force),&(cup->translational_force));
            cup->id=client->id;
            printf("--- Vehicle with id: %d x: %f, y: %f, theta:%f --- \n",cup->id,cup->x,cup->y,cup->theta);
            client = client->next;
        }

        int size=Packet_serialize(buf_send,&wup->header);
        if(size==0 || size==-1){
            pthread_mutex_unlock(&mutex);
            sleep(1);
            continue;
			}
        client=users->first;
        while(client!=NULL){
            if(client->isAddrReady==1){
                    int ret = sendto(socket_udp, buf_send, size, 0, (struct sockaddr*) &client->user_addr, (socklen_t) sizeof(client->user_addr));
                    debug_print("[UDP_Send] Sent WorldUpdate of %d bytes to client with id %d \n",ret,client->id);
                }
            client=client->next;
            }
        Packet_free(&(wup->header));
        fprintf(stdout,"[UDP_Send] WorldUpdatePacket sent to each client \n");
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    pthread_exit(NULL);
}

//Remove client that are not sending updates and the one that are AFK in the same place for an extended period of time
void* garbage_collector(void* args){
    debug_print("[GC] Garbage collector initialized \n");
    int socket_udp=*(int*)args;
    while(cleanGarbage){
        if(hasUsers==0) goto END;
        pthread_mutex_lock(&mutex);
        ClientListItem* client=users->first;
        long current_time=(long)time(NULL);
        int count=0;
        while(client!=NULL){
            long creation_time=(long)client->creation_time;
            long last_update_time=(long)client->last_update_time;
            if((client->isAddrReady==1 && (current_time-last_update_time)>MAX_TIME_WITHOUT_VEHICLEUPDATE) || (client->isAddrReady!=1 && (current_time-creation_time)>MAX_TIME_WITHOUT_VEHICLEUPDATE)){
                ClientListItem* tmp=client;
                client=client->next;
                sendDisconnect(socket_udp,tmp->user_addr);
                ClientListItem* del=ClientList_detach(users,tmp);
                if (del==NULL) continue;
                if(!del->insideWorld) goto SKIP;
                World_detachVehicle(&serverWorld,del->vehicle);
                Vehicle_destroy(del->vehicle);
                Image* user_texture=del->v_texture;
                if (user_texture!=NULL) Image_free(user_texture);
                count++;
                if(users->size==0) hasUsers=0;
                SKIP: close(del->id);
                free(del);
            }
            else client=client->next;
        }
        if (count>0) fprintf(stdout,"[GC] Removed %d users from the client list \n",count);
        END: pthread_mutex_unlock(&mutex);
        sleep(10);
    }
    pthread_exit(NULL);
}

void* tcp_auth(void* args){
    tcp_args* arg=(tcp_args*)args;
    int sockaddr_len = sizeof(struct sockaddr_in);
    while (connectivity) {
        struct sockaddr_in client_addr = {0};
        // Setup to accept client connection
        int client_desc = accept(server_tcp, (struct sockaddr*)&client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) {
            debug_print("Errore");
            continue;
        }
        else if(client_desc==-1) break;
        tcp_args tcpArgs;
        pthread_t threadTCP;
        tcpArgs.client_desc=client_desc;
        tcpArgs.elevation_texture = arg->elevation_texture;
        tcpArgs.surface_texture = arg->surface_texture;
        //Create a thread for each client
        int ret = pthread_create(&threadTCP, NULL,tcp_flow, &tcpArgs);
        PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread tcp failed");
        ret = pthread_detach(threadTCP);
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
    long tmp = strtol(argv[3], NULL, 0);
    if (tmp < 1024 || tmp > 49151) {
        fprintf(stderr, "Use a port number between 1024 and 49151.\n");
        exit(EXIT_FAILURE);
        }

    fprintf(stdout,"[Main] loading vehicle texture from %s ... ", argv[1]);
    Image* my_texture = Image_load("./images/arrow-right.ppm");
    if (my_texture) {
        printf("Done! \n");
        }
    else {
        printf("Fail! \n");
        }


    // load the images
    fprintf(stdout,"[Main] loading elevation image from %s ... ", elevation_filename);
    Image* surface_elevation = Image_load(elevation_filename);
    if (surface_elevation) {
        fprintf(stdout,"Done! \n");
        }
    else {
        fprintf(stdout,"Fail! \n");
    }
    fprintf(stdout,"[Main] loading texture image from %s ... ", texture_filename);
    Image* surface_texture = Image_load(texture_filename);
    if (surface_texture) {
        fprintf(stdout,"Done! \n");
    }
    else {
        fprintf(stdout,"Fail! \n");
    }

    port_number_no = htons((uint16_t)tmp);

    // setup tcp socket
    debug_print("[Main] Starting TCP socket \n");

    server_tcp = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(server_tcp, "Can't create server_tcp socket");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;

    int reuseaddr_opt = 1; // recover server if a crash occurs
    ret = setsockopt(server_tcp, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Failed setsockopt() on server_tcp socket");

    int sockaddr_len = sizeof(struct sockaddr_in);
    ret = bind(server_tcp, (struct sockaddr*) &server_addr, sockaddr_len); // binding dell'indirizzo
    ERROR_HELPER(ret, "Failed bind() on server_tcp");

    ret = listen(server_tcp, 16); // flag socket as passive
    ERROR_HELPER(ret, "Failed listen() on server_desc");

    debug_print("[Main] TCP socket successfully created \n");

    //init List structure
    users = malloc(sizeof(ListHead));
	ClientList_init(users);
    fprintf(stdout,"[Main] Initialized users list \n");

    //seting signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigfillset(&sa.sa_mask);
    ret=sigaction(SIGHUP, &sa, NULL);
    ERROR_HELPER(ret,"Error: cannot handle SIGHUP");
    ret=sigaction(SIGINT, &sa, NULL);
    ERROR_HELPER(ret,"Error: cannot handle SIGINT");

    debug_print("[Main] Custom signal handlers are now enabled \n");

    //setup UDP socket

    uint16_t port_number_no_udp= htons((uint16_t)UDPPORT);
    server_udp = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(server_udp, "Can't create server_udp socket");

    struct sockaddr_in udp_server = {0};
    udp_server.sin_addr.s_addr = INADDR_ANY;
    udp_server.sin_family      = AF_INET;
    udp_server.sin_port        = port_number_no_udp;

    int reuseaddr_opt_udp = 1; // recover server if a crash occurs
    ret = setsockopt(server_udp, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt_udp, sizeof(reuseaddr_opt_udp));
    ERROR_HELPER(ret, "Failed setsockopt() on server_udp socket");

    ret = bind(server_udp, (struct sockaddr*) &udp_server, sizeof(udp_server)); // binding dell'indirizzo
    ERROR_HELPER(ret, "Failed bind() on server_udp socket");

    debug_print("[Main] UDP socket created \n");
    tcp_args tcpArgs;
    tcpArgs.surface_texture=surface_texture;
    tcpArgs.elevation_texture=surface_elevation;
    World_init(&serverWorld, surface_elevation, surface_texture,  0.5, 0.5, 0.5);
    Vehicle* vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, &serverWorld, 0, my_texture);
    World_addVehicle(&serverWorld, vehicle);

    pthread_t UDP_receiver,UDP_sender,GC_thread,tcp_thread;
    ret = pthread_create(&UDP_receiver, NULL,udp_receiver, &server_udp);
    PTHREAD_ERROR_HELPER(ret, "pthread_create on thread tcp failed");
    ret = pthread_create(&UDP_sender, NULL,udp_sender, &server_udp);
    PTHREAD_ERROR_HELPER(ret, "pthread_create on thread tcp failed");
    ret = pthread_create(&GC_thread, NULL,garbage_collector, &server_udp);
    PTHREAD_ERROR_HELPER(ret, "pthread_create on garbace collector thread failed");
    ret = pthread_create(&tcp_thread, NULL,tcp_auth, &tcpArgs);
    PTHREAD_ERROR_HELPER(ret, "pthread_create on garbace collector thread failed");

    WorldViewer_runGlobal(&serverWorld, vehicle, &argc, argv);
    //creating server world
    connectivity=0;
    exchangeUpdate=0;

    fprintf(stdout,"[Main] World created. Now waiting for clients to connect...");

    fprintf(stdout,"[Main] Shutting down the server... \n");
    //Wait for threads to finish
    ret=pthread_join(UDP_receiver,NULL);
    ERROR_HELPER(ret,"Join on UDP_receiver thread failed");
    ret=pthread_join(tcp_thread,NULL);
    ERROR_HELPER(ret,"Join on tcp_auth thread failed");
    debug_print("[Main] UDP_receiver ended... \n");
    ret=pthread_join(UDP_sender,NULL);
    ERROR_HELPER(ret,"Join on UDP_sender thread failed");
    debug_print("[Main] UDP_sender ended... \n");
    ret=pthread_join(GC_thread,NULL);
    ERROR_HELPER(ret,"Join on garbage collector thread failed");

    debug_print("[Main] GC ended... \n");
    debug_print("[Main] Freeing resources... \n");
    //Delete list
    pthread_mutex_lock(&mutex);
    ClientList_destroy(users);
    pthread_mutex_unlock(&mutex);

    //Close descriptors
    ret = close(server_tcp);
    ERROR_HELPER(ret,"Failed close() on server_tcp socket");
    ret = close(server_udp);
    ERROR_HELPER(ret,"Failed close() on server_udp socket");
    Image_free(surface_elevation);
	Image_free(surface_texture);
    exit(EXIT_SUCCESS);
}
