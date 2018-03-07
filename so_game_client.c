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

void handle_signal(int signal){
    // Find out which signal we're handling
    switch (signal) {
        case SIGHUP:
            break;
        case SIGINT:
            connectivity=0;
            checkUpdate=0;
            sendGoodbye(socket_desc, id);
            break;
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }
}


int createUDPSocket(struct sockaddr_in* si_me, int port){
    int socket_udp;
    socket_udp=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ERROR_HELPER(socket_udp,"Something went wrong during the creation of the udp socket");
    memset((char *) &si_me, 0, sizeof(si_me));
    in_addr_t ip_addr = inet_addr(SERVER_ADDRESS);
    si_me->sin_family = AF_INET;
    si_me->sin_port = htons(port);
    si_me->sin_addr.s_addr = ip_addr;
    int ret=bind(socket_udp, (struct sockaddr*)&si_me, sizeof(si_me) );
    ERROR_HELPER(ret,"Error during bind");
    return socket_udp;
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
    //ret=sigaction(SIGINT, &sa, NULL);
    //ERROR_HELPER(ret,"Error: cannot handle SIGINT");


    debug_print("[Main] Starting ID,map_elevation,map_texture requests \n");
    int id=getID(socket_desc);
    debug_print("[Main] ID received \n");
    Image* map_elevation=getElevationMap(socket_desc);
    debug_print("[Main] Map elevation received \n");
    Image* map_texture = getTextureMap(socket_desc);
    debug_print("[Main] Map texture received \n");
    debug_print("[Main] Sending vehicle texture");
    sendVehicleTexture(socket_desc,my_texture,id);
    debug_print("[Main] Client Vehicle texture sent \n");


    // construct the world
    World_init(&world, map_elevation, map_texture,0.5, 0.5, 0.5);
    vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, &world, id, my_texture);
    World_addVehicle(&world, vehicle);

    // spawn a thread that will listen the update messages from
    // the server, and sends back the controls
    // the update for yourself are written in the desired_*_force
    // fields of the vehicle variable
    // when the server notifies a new player has joined the game
    // request the texture and add the player to the pool

    WorldViewer_runGlobal(&world, vehicle, &argc, argv);
    // check out the images not needed anymore

    Image_free(my_texture);
    Image_free(map_elevation);
    Image_free(map_texture);
    // world cleanup
    World_destroy(&world);
    return 0;
}
