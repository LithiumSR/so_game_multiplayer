
// #include <GL/glut.h> // not needed here
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"

void parent(int client_desc, struct sockaddr_in* client_addr){
    //close unused fd
	int ret;
	ret = close(client_desc);
	ERROR_HELPER(ret, "[server] ERRORE: impossibile chiudere la client socket");

	// resetto i campi in client_addr
	memset(client_addr, 0, sizeof(struct sockaddr_in));
}

void child(int socket_desc, int client_desc, struct sockaddr_in* client_addr){
	int ret;
	ret = close(socket_desc);
	ERROR_HELPER(ret, "[server] ERRORE: impossibile chiudere la client socket");
    printf("[child] gestisco comunicazione con processo client.\n");
	connection_handler(client_desc, client_addr);
	printf("[child] conversazione con client terminata.\n");

	free(client_addr);
	exit(EXIT_SUCCESS);
}

void connection_handler(int socket_desc, struct sockaddr_in* client_addr) {
    //WIP
}




int main(int argc, char **argv) {
  if (argc<4) {
    printf("usage: %s <elevation_image> <texture_image> <port_number>\n", argv[1]);
    exit(-1);
  }
  char* elevation_filename=argv[1];
  char* texture_filename=argv[2];
  char* vehicle_texture_filename="./images/arrow-right.ppm";
  printf("loading elevation image from %s ... ", elevation_filename);

  // load the images
  Image* surface_elevation = Image_load(elevation_filename);
  if (surface_elevation) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }


  printf("loading texture image from %s ... ", texture_filename);
  Image* surface_texture = Image_load(texture_filename);
  if (surface_texture) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }

  printf("loading vehicle texture (default) from %s ... ", vehicle_texture_filename);
  Image* vehicle_texture = Image_load(vehicle_texture_filename);
  if (vehicle_texture) {
    printf("Done! \n");
  } else {
    printf("Fail! \n");
  }


    uint16_t port_number_no;
    long tmp = strtol(argv[3], NULL, 0); // convert a string to long
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

        pid_t pid;
		pid=fork();
		ERROR_HELPER(pid, "[server] ERRORE: impossibile eseguire fork");
		if (pid==0){
			parent(client_desc,client_addr);
		}
		else {
			child(socket_desc, client_desc, client_addr);
		}
        // Close connection
        ret = close(client_desc);
        ERROR_HELPER(ret, "[MAIN THREAD] Impossibile chiudere la socket client_desc per la connessione accettata");

    }

  exit(EXIT_SUCCESS); // mai eseguita
}
