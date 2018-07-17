#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/common.h"
#include "../game_framework/client_list.h"
#include "../game_framework/so_game_protocol.h"
int main(int argc, char const* argv[]) {
  printf("Starting client list tests...");
  ClientListHead* test = (ClientListHead*)malloc(sizeof(ClientListHead));
  ClientList_print(test);

  ClientListItem* u1 = (ClientListItem*)malloc(sizeof(ClientListItem));
  u1->id = 1;
  u1->v_texture = NULL;
  printf("Add element with id 1...");
  ClientList_insert(test, u1);
  ClientList_print(test);
  ClientListItem* u2 = (ClientListItem*)malloc(sizeof(ClientListItem));
  u2->id = 2;
  u2->v_texture = NULL;
  printf("Add element with id 2...");
  ClientList_insert(test, u2);
  ClientList_print(test);
  ClientListItem* cil = ClientList_findByID(test, 2);
  printf("Finding and comparing elements with id 2...");
  if (cil == NULL) {
    printf("ERROR IN DETACH \n");
    return -1;
  }
  if (cil == u2) printf("OK \n");
  ClientListItem* cil2 = ClientList_findByID(test, 1);
  printf("Finding and comparing elements with id 1...");
  if (cil2 == NULL) {
    printf("ERROR IN DETACH \n");
    return -1;
  }
  if (cil2 == u1) printf("OK \n ");
  printf("Removing elements with id 2... \n");
  ClientList_detach(test, cil2);
  ClientList_print(test);
  printf("Removing elements with id 1... \n");
  ClientList_detach(test, cil);
  ClientList_print(test);
  printf("Readding elements previously removed... \n");
  ClientList_insert(test, u1);
  ClientList_insert(test, u2);
  printf("Destroying resources for good... \n");
  ClientList_destroy(test);
  fflush(stdout);
}
