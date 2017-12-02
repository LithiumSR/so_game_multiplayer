
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <errno.h>
#include "common.h"

#define SERVER_PORT     3000

void session(int sock_fd) {
    int ret;
    char buf[1024]= "test :)";
    int bytes_sent=0;
    int msg_len=strlen(buf);
    while(bytes_sent<msg_len){
			ret=send(sock_fd,buf+bytes_sent,msg_len-bytes_sent,0);
			if (ret==-1 && errno==EINTR) continue;
			ERROR_HELPER(ret,"Errore invio");
			if (ret==0) break;
			bytes_sent+=ret;
    }
    printf("[SESSION] Messaggio inviato");
}


int main(int argc, char **argv) {

 if (argc != 2) {
       fprintf(stderr, "Sintassi: %s <port_number>\n", argv[0]);
       exit(EXIT_FAILURE);
    }

    int ret;

    // Get port number
    uint16_t port_number_no;
    long tmp = strtol(argv[1], NULL, 0); // convert a string to long
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

        // Process request
        session(client_desc);

        // Close connection
        ret = close(client_desc);
        ERROR_HELPER(ret, "[MAIN THREAD] Impossibile chiudere la socket client_desc per la connessione accettata");

    }

}

