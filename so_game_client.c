
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
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
#include "so_game_protocol.h"
#define PORT 8888
#define SERVER "127.0.0.1"
#define BUFLEN 3000
int window;
World world;
Vehicle* vehicle; // The vehicle
int id;
uint16_t  port_number_no;

void getUpdates(void){
    int nBytes;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    char buffer[BUFLEN];
    int clientSocket=socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(clientSocket,"Errore creazione socket UDP");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    addr_size = sizeof serverAddr;
    while(1){
        printf("Type a sentence to send to server:\n");
        char* ret=fgets(buffer,1024,stdin);
        if (ret==NULL) ERROR_HELPER(-1,"Errore fgets");
        printf("You typed: %s",buffer);
        nBytes = strlen(buffer) + 1;
        nBytes = recvfrom(clientSocket,buffer,1024,0,NULL, NULL);
        printf("Received from server: %s\n",buffer);
  }
}

void sendUpdates(void){
    int nBytes;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    char buffer[BUFLEN];
    int clientSocket=socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(clientSocket,"Errore creazione socket UDP");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    addr_size = sizeof serverAddr;

    while(1){
        //VehicleForces* vf=malloc(sizeof(VehicleForces);
        sendto(clientSocket,buffer,nBytes,0,(struct sockaddr *)&serverAddr,addr_size);
    }

}

int main(int argc, char **argv) {
    if (argc<4) {
        printf("usage: %s <server_address> <player texture> <port-server>\n", argv[1]);
        exit(-1);
        }
    printf("loading texture image from %s ... ", argv[2]);
    Image* my_texture = Image_load(argv[2]);
    if (my_texture) {
        printf("Done! \n");
        }
    else {
        printf("Fail! \n");
        }

  // todo: connect to the server
  //   -get ad id
  //   -send your texture to the server (so that all can see you)
  //   -get an elevation map
  //   -get the texture of the surface

  //Image* my_texture_from_server;
    long tmp = strtol(argv[1], NULL, 0);
    if (tmp < 1024 || tmp > 49151) {
        fprintf(stderr, "Use a port number between 1024 and 49151.\n");
        exit(EXIT_FAILURE);
        }

    port_number_no = htons((uint16_t)tmp); // we use network byte order
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    in_addr_t ip_addr = inet_addr(SERVER);
    ERROR_HELPER(socket_desc, "Impossibile creare una socket");
	struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0
    server_addr.sin_addr.s_addr = ip_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;
	int ret= connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Impossibile connettersi al server");
    printf("Connessione al server stabilita!\n");

    //We will see if are needed
    //int msg_len=0;
    //int buf_len=1000000;
    //char* buf=(char*)malloc(sizeof(char)*buf_len);

    //Get ID
    int id=get_client_ID(socket_desc);
    printf("[Main] Richiesta ID Completata");
    Image* map_elevation=get_image_elevation(socket_desc,id);
    printf("[Main] Richiesta map_elevation completata");
    Image* map_texture = get_image_texture(socket_desc,id);
    printf("[Main] Richiesta map_texture completata");

    // construct the world
    World_init(&world, map_elevation, map_texture,0.5, 0.5, 0.5);
    vehicle=(Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, &world, id, my_texture);
    World_addVehicle(&world, vehicle);

    close(socket_desc);
    //No longer needed
    // spawn a thread that will listen the update messages from
  // the server, and sends back the controls
  // the update for yourself are written in the desired_*_force
  // fields of the vehicle variable
  // when the server notifies a new player has joined the game
  // request the texture and add the player to the pool
  /*FILLME*/

    pthread_t thread_background1, thread_background2;
    ret = pthread_create(&thread_background1,NULL, (void*)&getUpdates, NULL);
    if (ret!=0) ERROR_HELPER(-1,"Something went wrong when creating the thread");
    ret = pthread_create(&thread_background2, NULL, (void*)&sendUpdates, NULL);
    if (ret!=0) ERROR_HELPER(-1,"Something went wrong when creating the thread");


    WorldViewer_runGlobal(&world, vehicle, &argc, argv);
  // check out the images not needed anymore
    Image_free(my_texture);
    Image_free(map_elevation);
    Image_free(map_texture);

  // world cleanup
  //Vehicle_destroy(vehicle); Not needed because world destroy does that for me
    World_destroy(&world);
    return 0;
}
