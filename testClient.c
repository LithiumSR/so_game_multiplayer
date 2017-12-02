
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <errno.h>
#include "common.h"

#define SERVER_PORT     2015

int main(int argc, char **argv) {
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
    int msg_len=0;
    char* buf[50];
    int buf_len=50;
    while ( (msg_len = recv(socket_desc, buf, buf_len, 0)) < 0 ) {
            if (errno == EINTR) continue;
            ERROR_HELPER(-1, "Cannot read from socket");
        }
    printf("%s",(char*)buf);

}

