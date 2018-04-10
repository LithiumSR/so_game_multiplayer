#include <arpa/inet.h>  // htons() and inet_addr()
#include <errno.h>
#include <netinet/in.h>  // struct sockaddr_in
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../common.h"
#include "../so_game_protocol.h"
#define BUFFSIZE 1000000

int main(int argc, char **argv) {
  long tmp = strtol(argv[1], NULL, 0);
  if (tmp < 1024 || tmp > 49151) {
    fprintf(stderr, "Use a port number between 1024 and 49151.\n");
    exit(EXIT_FAILURE);
  }
  uint16_t port_number_no = htons((uint16_t)tmp);  // we use network byte order
  int socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
  in_addr_t ip_addr = inet_addr(SERVER_ADDRESS);
  ERROR_HELPER(socket_desc, "Impossibile creare una socket");
  struct sockaddr_in udp_server = {
      0};  // some fields are required to be filled with 0
  udp_server.sin_addr.s_addr = ip_addr;
  udp_server.sin_family = AF_INET;
  udp_server.sin_port = port_number_no;
  PacketHeader ph;
  ph.type = GetId;
  IdPacket *ip = (IdPacket *)malloc(sizeof(IdPacket));
  ip->header = ph;
  char buf_send[BUFFSIZE];

  int size = Packet_serialize(buf_send, &(ip->header));
  while (1) {
    int sent =
        sendto(socket_desc, buf_send, size, 0,
               (const struct sockaddr *)&(udp_server), sizeof(udp_server));
    ERROR_HELPER(sent, "Errore send");
    debug_print("Inviato %d bytes over UDP \n", sent);
    sleep(2);
  }

  return 0;
}
