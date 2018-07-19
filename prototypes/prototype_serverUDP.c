#include <arpa/inet.h>  // htons() and inet_addr()
#include <errno.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../common/common.h"
#include "../game_framework/protogame_protocol.h"
#define BUFFSIZE 1000000

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Sintassi: %s <port_number>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int ret;
  // Get port number
  uint16_t port_number_no;
  long tmp = strtol(argv[1], NULL, 0);  // convert a string to long
  if (tmp < 1024 || tmp > 49151) {
    fprintf(stderr,
            "Utilizzare un numero di porta compreso tra 1024 e 49151.\n");
    exit(EXIT_FAILURE);
  }
  port_number_no = htons((uint16_t)tmp);

  // setup server
  int server_desc = socket(AF_INET, SOCK_DGRAM, 0);
  ERROR_HELPER(server_desc,
               "[MAIN THREAD] Impossibile creare socket server_desc");

  struct sockaddr_in udp_server = {0};
  udp_server.sin_addr.s_addr = INADDR_ANY;
  udp_server.sin_family = AF_INET;
  udp_server.sin_port = port_number_no;

  int reuseaddr_opt = 1;  // recover server if a crash occurs
  ret = setsockopt(server_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt,
                   sizeof(reuseaddr_opt));
  ERROR_HELPER(ret,
               "[MAIN THREAD] Impossibile settare l'opzione SO_REUSEADDR "
               "per socket server_desc");

  ret = bind(server_desc, (struct sockaddr *)&udp_server,
             sizeof(udp_server));  // binding dell'indirizzo
  ERROR_HELPER(ret, "[MAIN THREAD] Impossibile eseguire bind() su server_desc");
  Image *texture_map = Image_load("./images/test.pgm");
  if (texture_map == NULL) ERROR_HELPER(-1, "Errore caricamento texture map");
  Image *elevation_map = Image_load("./images/test.ppm");
  if (elevation_map == NULL)
    ERROR_HELPER(-1, "Errore caricamento elevation map");
  socklen_t addrlen = sizeof(struct sockaddr_in);
  int bytes_received = 0;
  while (1) {
    char buf_recv[BUFFSIZE];
    struct sockaddr_in client_addr = {0};
    bytes_received = recvfrom(server_desc, buf_recv, BUFFSIZE, 0,
                              (struct sockaddr *)&client_addr, &addrlen);
    debug_print("Ho ricevuto %d bytes over UDP \n", bytes_received);
  }
  printf("Esco");
}
