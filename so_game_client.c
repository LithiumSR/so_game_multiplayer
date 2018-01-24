
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include "common.h"
#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "client_op.h"
#include "so_game_protocol.h"

int window;
World world;
Vehicle* vehicle; // The vehicle
int id;

int main(int argc, char **argv) {
  if (argc<4) {
    printf("usage: %s <server_address> <player texture> <port-server>\n", argv[1]);
    exit(-1);
  }

  printf("loading texture image from %s ... ", argv[2]);
  Image* my_texture = Image_load(argv[2]);
  if (my_texture) {
    printf("Done! \n");
  } else {
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
    uint16_t port_number_no = htons((uint16_t)tmp); // we use network byte order
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    in_addr_t ip_addr = inet_addr("127.0.0.1");
    ERROR_HELPER(socket_desc, "Impossibile creare una socket");
	struct sockaddr_in server_addr = {0}; // some fields are required to be filled with 0
    server_addr.sin_addr.s_addr = ip_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number_no;
	int ret= connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Impossibile connettersi al server");
    fprintf(stderr, "Connessione al server stabilita!\n");
    //We will see if are needed
    //int msg_len=0;
    //int buf_len=1000000;
    //char* buf=(char*)malloc(sizeof(char)*buf_len);

    //Get ID
    ret=send_request_type(socket_desc,GetId);
    ERROR_HELPER(ret,"[Main] Richiesta ID Fallita");
    IdPacket* pk=get_next_IdPacket(socket_desc);
    id=pk->id;
    free(pk);

    //Get map_elevation
    ret=send_request_type(socket_desc,GetElevation);
    ERROR_HELPER(ret,"[Main] Richiesta GetElevation Fallita");
    ImagePacket* packet_elevation=get_next_ImagePacket(socket_desc);
    Image* map_elevation=packet_elevation->image;

    //Get map_texture
    ret=send_request_type(socket_desc,GetTexture);
    ERROR_HELPER(ret,"[Main] Richiesta GetTexture Fallita");
    ImagePacket* packet_map_texture=get_next_ImagePacket(socket_desc);
    Image* map_texture=packet_map_texture->image;

    //send texture
    ret=send_client_Texture(socket_desc,my_texture,id);
    ERROR_HELPER(ret,"[Main] Invio texture fallito");


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
  /*FILLME*/

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
