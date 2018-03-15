#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "common.h"
#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "client_op.h"
#include "so_game_protocol.h"
#include <fcntl.h>
#define NO_ACCESS -2

int window;
World world;
Vehicle* vehicle; // The vehicle
int id;
uint16_t  port_number_no;
int connectivity=1;
int exchangeUpdate=1;
int socket_desc; //socket tcp
time_t last_update_time=-1;

typedef struct localWorld{
    int  ids[WORLDSIZE];
    int users_online;
    char hasVehicle[WORLDSIZE];
    Vehicle** vehicles;
}localWorld;


typedef struct listenArgs{
    localWorld* lw;
    struct sockaddr_in server_addr;
    int socket_udp;
    int socket_tcp;
}udpArgs;

void handle_signal(int signal){
    // Find out which signal we're handling
    switch (signal) {
        case SIGHUP:
            break;
        case SIGINT:
            connectivity=0;
            exchangeUpdate=0;
            if(last_update_time!=1) sendGoodbye(socket_desc, id);
            exit(0);
            break;
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }
}

int add_user_id(int ids[] , int size , int id2, int* position,int* users_online){
    if(*users_online==WORLDSIZE){
        *position=-1;
        return -1;
    }
    for(int i=0;i<size;i++){
        if(ids[i]==id2) {
            return i;
        }
    }
    for (int i=0 ; i < size ; i++){
        if(ids[i]==-1){
            ids[i]=id2;
            *users_online+=1;
            *position=i;
            break;
        }
    }
    return -1;
}


int sendUpdates(int socket_udp,struct sockaddr_in server_addr,int serverlen){
    char buf_send[BUFFERSIZE];
    PacketHeader ph;
    ph.type=VehicleUpdate;
    VehicleUpdatePacket* vup=(VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
    vup->header=ph;
    vup->time=time(NULL);
    getForcesUpdate(vehicle,&(vup->translational_force),&(vup->rotational_force));
    vup->id=id;
    vup->time=time(NULL);
    int size=Packet_serialize(buf_send, &vup->header);
    int bytes_sent = sendto(socket_udp, buf_send, size, 0, (const struct sockaddr *) &server_addr,(socklen_t) serverlen);
    debug_print("[UDP_Sender] Sent a VehicleUpdatePacket of %d bytes with tf:%f rf:%f \n",bytes_sent,vup->translational_force,vup->rotational_force);
    Packet_free(&(vup->header));
    time_t current_time=time(NULL);
    if(last_update_time!=-1 && current_time-last_update_time>MAX_TIME_WITHOUT_WORLDUPDATE){
        connectivity=0;
        exchangeUpdate=0;
        fprintf(stdout,"[WARNING] Server is not avaiable. Terminating the client now...");
        exit(0);
    }
    if(bytes_sent<0) return -1;
    return 0;
}

//Send vehicleUpdatePacket to server
void* udp_sender(void* args){
    udpArgs udp_args =*(udpArgs*)args;
    struct sockaddr_in server_addr=udp_args.server_addr;
    int socket_udp =udp_args.socket_udp;
    int serverlen=sizeof(server_addr);
    while(connectivity && exchangeUpdate){
        int ret=sendUpdates(socket_udp,server_addr,serverlen);
        if(ret==-1) debug_print("[UDP_Sender] Cannot send VehicleUpdatePacket \n");
        sleep(1);
    }
    pthread_exit(NULL);
}

//Receive and apply WorldUpdatePacket from server
void* udp_receiver(void* args){
    udpArgs udp_args =*(udpArgs*)args;
    struct sockaddr_in server_addr=udp_args.server_addr;
    int socket_udp =udp_args.socket_udp;
    socklen_t addrlen= sizeof(server_addr);
    localWorld* lw=udp_args.lw;
    int socket_tcp=udp_args.socket_tcp;
    while(connectivity && exchangeUpdate){
        char buf_rcv[BUFFERSIZE];
        int bytes_read=recvfrom(socket_udp, buf_rcv, BUFFERSIZE, 0, (struct sockaddr*) &server_addr, &addrlen);
        if(bytes_read==-1){
            debug_print("[UDP_Receiver] Can't receive Packet over UDP \n");
            usleep(300);
            continue;
        }
        if (bytes_read==0) {
            usleep(300);
            continue;
        }

        debug_print("[UDP_Receiver] Received %d bytes over UDP\n",bytes_read);
        PacketHeader* ph=(PacketHeader*)buf_rcv;
        if(ph->size!=bytes_read){
            debug_print("[WARNING] Skipping partial UDP packet \n");
            usleep(300);
            continue;
        }
        if(ph->type==PostDisconnect){
            fprintf(stdout,"[WARNING] You were kicked out of the server for inactivity... Closing the client now \n");
            sendGoodbye(socket_desc, id);
            connectivity=0;
            exchangeUpdate=0;
            exit(0);
        }

        if(ph->type!=PostDisconnect && ph->type!=WorldUpdate){
            fprintf(stdout,"[UDP_Receiver] Found an unknown udp packet. Terminating the client now... \n");
            sendGoodbye(socket_desc, id);
            connectivity=0;
            exchangeUpdate=0;
            exit(-1);
        }
        WorldUpdatePacket* wup = (WorldUpdatePacket*)Packet_deserialize(buf_rcv, bytes_read);
        debug_print("WorldUpdatePacket contains %d vehicles besides mine \n",wup->num_vehicles-1);
        last_update_time=wup->time;
        char mask[WORLDSIZE];
        for(int k=0;k<WORLDSIZE;k++) mask[k]=NO_ACCESS;
        float x,y,theta;
        getXYTheta(vehicle,&x,&y,&theta);
        int ignored=0;

        for(int i=0; i < wup -> num_vehicles ; i++){
            //if(wup->updates[i].id==id) continue;
            if(wup->updates[i].id!=id && (abs((int)x-(int)wup->updates[i].x)>HIDE_RANGE || abs((int)y-(int)wup->updates[i].y))>HIDE_RANGE) {
                ignored++;
                continue;
            }
            int new_position=-1;
            int id_struct=add_user_id(lw->ids,WORLDSIZE,wup->updates[i].id,&new_position,&(lw->users_online));
            if(id_struct==-1){
                if(new_position==-1) continue;
                mask[new_position]=1;
                fprintf(stdout,"New Vehicle with id %d and x: %f y: %f z: %f \n",wup->updates[i].id,wup->updates[i].x,wup->updates[i].y,wup->updates[i].theta);
                Image* img = getVehicleTexture(socket_tcp,wup->updates[i].id);
                Vehicle* new_vehicle=(Vehicle*) malloc(sizeof(Vehicle));
                Vehicle_init(new_vehicle,&world,wup->updates[i].id,img);
                lw->vehicles[new_position]=new_vehicle;
                setXYTheta(lw->vehicles[new_position],wup->updates[i].x,wup->updates[i].y,wup->updates[i].theta);
                //setForces(lw->vehicles[new_position],wup->updates[i].translational_force,wup->updates[i].rotational_force);
                World_addVehicle(&world, new_vehicle);
                lw->hasVehicle[new_position]=1;
            }
            else {
                mask[id_struct]=1;
                if(wup->updates[i].forceRefresh==1 && wup->updates[i].id!=id){
                    debug_print("[WARNING] Forcing refresh for client with id %d",wup->updates[i].id);
                    Image* im=lw->vehicles[id_struct]->texture;
                    World_detachVehicle(&world,lw->vehicles[id_struct]);
                    if (im!=NULL) Image_free(im);
                    Image* img = getVehicleTexture(socket_tcp,wup->updates[i].id);
                    Vehicle_destroy(lw->vehicles[id_struct]);
                    free(lw->vehicles[id_struct]);
                    Vehicle* new_vehicle=(Vehicle*) malloc(sizeof(Vehicle));
                    Vehicle_init(new_vehicle,&world,wup->updates[i].id,img);
                    lw->vehicles[id_struct]=new_vehicle;
                    setXYTheta(lw->vehicles[id_struct],wup->updates[i].x,wup->updates[i].y,wup->updates[i].theta);
                    //setForces(lw->vehicles[id_struct],wup->updates[i].translational_force,wup->updates[i].rotational_force);
                    World_addVehicle(&world, new_vehicle);
                    lw->hasVehicle[id_struct]=1;
                    continue;
                }
                fprintf(stdout,"Updating Vehicle with id %d and x: %f y: %f z: %f \n",wup->updates[i].id,wup->updates[i].x,wup->updates[i].y,wup->updates[i].theta);
                if(wup->updates[i].id==id) setXYTheta(lw->vehicles[0],wup->updates[i].x,wup->updates[i].y,wup->updates[i].theta);
                else setXYTheta(lw->vehicles[id_struct],wup->updates[i].x,wup->updates[i].y,wup->updates[i].theta);
                //setForces(lw->vehicles[id_struct],wup->updates[i].translational_force,wup->updates[i].rotational_force);
            }
        }
        if (ignored>0) debug_print("[INFO] Ignored %d vehicles based on position \n",ignored);
        for(int i=0; i < WORLDSIZE ; i++){
            if(mask[i]==1) continue;
            if(i==0) continue;

            if(lw->ids[i]==id) continue;
            if(mask[i]==NO_ACCESS && lw->ids[i]!=-1){
                fprintf(stdout,"[WorldUpdate] Removing Vehicles with ID %d \n",lw->ids[i]);
                lw->users_online=lw->users_online-1;
                if(!lw->hasVehicle[i]) continue;
                Image* im=lw->vehicles[i]->texture;
                World_detachVehicle(&world,lw->vehicles[i]);
                if (im!=NULL) Image_free(im);
                Vehicle_destroy(lw->vehicles[i]);
                lw->ids[i]=-1;
                free(lw->vehicles[i]);
                lw->hasVehicle[i]=0;
            }
        }
        usleep(300);
    }
    pthread_exit(NULL);

}
int main(int argc, char **argv) {
    if (argc<3) {
        printf("usage: %s <player texture> <port_number> \n", argv[1]);
        exit(-1);
        }
    fprintf(stdout,"[Main] loading vehicle texture from %s ... ", argv[1]);
    Image* my_texture = Image_load(argv[1]);
    if (my_texture) {
        printf("Done! \n");
        }
    else {
        printf("Fail! \n");
        }
    long tmp= strtol(argv[2], NULL, 0);

    if (tmp < 1024 || tmp > 49151) {
      fprintf(stderr, "Use a port number between 1024 and 49151.\n");
      exit(EXIT_FAILURE);
      }

    fprintf(stdout,"[Main] Starting... \n");
    port_number_no = htons((uint16_t)tmp); // we use network byte order
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    in_addr_t ip_addr = inet_addr(SERVER_ADDRESS);
    ERROR_HELPER(socket_desc, "Cannot create socket \n");
	struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0
    server_addr.sin_addr.s_addr = ip_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;

    int reuseaddr_opt = 1; // recover server if a crash occurs
    int ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Can't set SO_REUSEADDR flag");

	ret= connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Cannot connect to remote server \n");
    debug_print("[Main] TCP connection established... \n");

    //seting signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;
    // Block every signal during the handler
    sigfillset(&sa.sa_mask);
    ret=sigaction(SIGHUP, &sa, NULL);
    ERROR_HELPER(ret,"Error: cannot handle SIGHUP");
    ret=sigaction(SIGINT, &sa, NULL);
    ERROR_HELPER(ret,"Error: cannot handle SIGINT");

    //setting up localWorld
    localWorld* myLocalWorld = (localWorld*)malloc(sizeof(localWorld));
    myLocalWorld->vehicles=(Vehicle**)malloc(sizeof(Vehicle*)*WORLDSIZE);
    for(int i=0;i<WORLDSIZE;i++){
        myLocalWorld->ids[i]=-1;
        myLocalWorld->hasVehicle[i]=0;
        #ifdef _USE_CACHE_TEXTURE_
        myLocalWorld->isDisabled[i]=0;
        #endif
    }

    //Talk with server
    fprintf(stdout,"[Main] Starting ID,map_elevation,map_texture requests \n");
    id=getID(socket_desc);
    myLocalWorld->ids[0]=id;
    fprintf(stdout,"[Main] ID number %d received \n",id);
    Image* surface_elevation=getElevationMap(socket_desc);
    fprintf(stdout,"[Main] Map elevation received \n");
    Image* surface_texture = getTextureMap(socket_desc);
    fprintf(stdout,"[Main] Map texture received \n");
    debug_print("[Main] Sending vehicle texture");
    sendVehicleTexture(socket_desc,my_texture,id);
    fprintf(stdout,"[Main] Client Vehicle texture sent \n");


    //create Vehicle
    World_init(&world, surface_elevation, surface_texture,0.5, 0.5, 0.5);
    vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, &world, id, my_texture);
    World_addVehicle(&world, vehicle);
    myLocalWorld->vehicles[0]=vehicle;
    myLocalWorld->hasVehicle[0]=1;
    if(SINGLEPLAYER) goto SKIP;

    //UDP Init
    uint16_t port_number_udp = htons((uint16_t)UDPPORT); // we use network byte order
	int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(socket_desc, "Can't create an UDP socket");
	struct sockaddr_in udp_server = {0}; // some fields are required to be filled with 0
    udp_server.sin_addr.s_addr = ip_addr;
    udp_server.sin_family      = AF_INET;
    udp_server.sin_port        = port_number_udp;
    debug_print("[Main] Socket UDP created and ready to work \n");

    //Create UDP Threads
    pthread_t UDP_sender,UDP_receiver;
    udpArgs udp_args;
    udp_args.socket_tcp=socket_desc;
    udp_args.server_addr=udp_server;
    udp_args.socket_udp=socket_udp;
    udp_args.lw=myLocalWorld;
    ret = pthread_create(&UDP_sender, NULL, udp_sender, &udp_args);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_sender");
    ret = pthread_create(&UDP_receiver, NULL, udp_receiver, &udp_args);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread UDP_receiver");

    //Disconnect from server if required by macro
    SKIP: if(SINGLEPLAYER) sendGoodbye(socket_desc,id);

    WorldViewer_runGlobal(&world, vehicle, &argc, argv);

    // Waiting threads to end and cleaning resources
    debug_print("[Main] Disabling and joining on UDP and TCP threads \n");
    connectivity=0;
    exchangeUpdate=0;
    if(!SINGLEPLAYER){
        ret=pthread_join(UDP_sender,NULL);
        PTHREAD_ERROR_HELPER(ret, "pthread_join on thread UDP_sender failed");
        ret=pthread_join(UDP_receiver,NULL);
        PTHREAD_ERROR_HELPER(ret, "pthread_join on thread UDP_receiver failed");
        ret= close(socket_udp);
        ERROR_HELPER(ret,"Failed to close UDP socket");
        }

    fprintf(stdout,"[Main] Cleaning up... \n");
    sendGoodbye(socket_desc,id);

    //Clean resources
    Image_free(surface_elevation);
    Image_free(surface_texture);
    for(int i=0;i<WORLDSIZE;i++){
        if(myLocalWorld->ids[i]==-1) continue;
        myLocalWorld->users_online--;
        Image* im=myLocalWorld->vehicles[i]->texture;
        World_detachVehicle(&world,myLocalWorld->vehicles[i]);
        if (im!=NULL) Image_free(im);
        Vehicle_destroy(myLocalWorld->vehicles[i]);
        free(myLocalWorld->vehicles[i]);
    }

    free(myLocalWorld->vehicles);
    free(myLocalWorld);
    ret=close(socket_desc);
    ERROR_HELPER(ret,"Failed to close TCP socket");
    if (!SINGLEPLAYER) ret=close(socket_udp);
    ERROR_HELPER(ret,"Failed to close UDP socket");
    // world cleanup
    World_destroy(&world);
    return 0;
}
