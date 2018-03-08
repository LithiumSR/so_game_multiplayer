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

int window;
World world;
Vehicle* vehicle; // The vehicle
int id;
uint16_t  port_number_no;
int connectivity=1;
int checkUpdate=1;
int socket_desc; //socket tcp

typedef struct localWorld{
    int  ids[WORLDSIZE];
    int users_online;
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
            checkUpdate=0;
            //sendGoodbye(socket_desc, id);
            exit(0);
            break;
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }
}

int sendUpdates(int socket_udp,struct sockaddr_in server_addr,int serverlen){
    char buf_send[BUFFERSIZE]="Ciao test test test test :) \n \0";
    int len= strlen(buf_send);
    debug_print("Sto per inviare %d bytes \n",len);
    int bytes_sent = sendto(socket_udp, buf_send, len, 0, (struct sockaddr *) &server_addr,(socklen_t) serverlen);
    debug_print("[UDP] Ho inviato %d \n",bytes_sent);
    if(bytes_sent<0) return -1;
    else {
        debug_print("[UDP_Sender] VehicleUpdatePacket sent");
        return 0;
    }
}

void* udp_test(void* args){
    udpArgs udp_args =*(udpArgs*)args;
    struct sockaddr_in server_addr=udp_args.server_addr;
    int socket_udp =udp_args.socket_udp;
    int serverlen=sizeof(server_addr);
    while(connectivity && checkUpdate){
        debug_print("[UDP_Test] Sto per inviare una stringa... \n");
        int ret=sendUpdates(socket_udp,server_addr,serverlen);
        if(ret==-1) debug_print("[UDP_Sender] Cannot send VehicleUpdatePacket");
        debug_print("[UDP_Test] Going to sleep");
        sleep(1000);
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc<3) {
        printf("usage: %s <player texture> <port_number> \n", argv[1]);
        exit(-1);
        }
    debug_print("[Main] loading vehicle texture from %s ... ", argv[1]);
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
    debug_print("[Main] Starting... \n");
    port_number_no = htons((uint16_t)tmp); // we use network byte order
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    in_addr_t ip_addr = inet_addr(SERVER_ADDRESS);
    ERROR_HELPER(socket_desc, "Cannot create socket \n");
	struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0
    server_addr.sin_addr.s_addr = ip_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;
	int ret= connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
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

    //Talk with server
    debug_print("[Main] Starting ID,map_elevation,map_texture requests \n");
    int id=getID(socket_desc);
    debug_print("[Main] ID number %d received \n",id);
    Image* surface_elevation=getElevationMap(socket_desc);
    debug_print("[Main] Map elevation received \n");
    Image* surface_texture = getTextureMap(socket_desc);
    debug_print("[Main] Map texture received \n");
    debug_print("[Main] Sending vehicle texture");
    sendVehicleTexture(socket_desc,my_texture,id);
    debug_print("[Main] Client Vehicle texture sent \n");
    
    struct sockaddr_in udp_addr;
    //UDP Init
    int socket_udp=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ERROR_HELPER(socket_udp,"Something went wrong during the creation of the udp socket");
    memset((char *) &udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(++tmp);
    udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ret=bind(socket_udp, (struct sockaddr*)&udp_addr, sizeof(udp_addr) );
    ERROR_HELPER(ret,"Error during bind");
    debug_print("Socket UDP created and ready to work \n");
    
    pthread_t UDP_test;
    udpArgs udp_args;
    udp_args.socket_tcp=socket_desc;
    udp_args.server_addr=udp_addr;
    udp_args.socket_udp=socket_udp;
    ret = pthread_create(&UDP_test, NULL,udp_test, &udp_args);
    PTHREAD_ERROR_HELPER(ret, "[MAIN] pthread_create on thread tcp failed");
    //Go offline if macro in common.h is set to 1
    if(OFFLINE) sendGoodbye(socket_desc,id);
    World_init(&world, surface_elevation, surface_texture,0.5, 0.5, 0.5);
    vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, &world, id, my_texture);
    World_addVehicle(&world, vehicle);
    WorldViewer_runGlobal(&world, vehicle, &argc, argv);
    // construct the world
    printf("Cleaning up... \n");
    fflush(stdout);
    sendGoodbye(socket_desc,id);
    Image_free(my_texture);
    Image_free(surface_elevation);
    Image_free(surface_texture);
    // world cleanup
    World_destroy(&world);
    return 0;
}
