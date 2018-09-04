#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/common.h"
#include "../game_framework/client_list.h"
#include "../game_framework/protogame_protocol.h"
int main(int argc, char const* argv[]) {
  printf("Starting client list tests...");
  char flag = 0;
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
    flag = -1;
  }
  if (cil == u2) printf("OK \n");
  ClientListItem* cil2 = ClientList_findByID(test, 1);
  printf("Finding and comparing elements with id 1...");
  if (cil2 == NULL) {
    printf("ERROR IN DETACH \n");
    flag = -1;
  }
  if (cil2 == u1) printf("OK \n ");
  printf("Removing elements with id 2...");
  ClientList_detach(test, u2);
  ClientListItem* remove2 = ClientList_findByID(test, 2);
  if (remove2 != NULL){
    printf("ERROR %d IN DETACH \n",remove2->id);
    flag = -1;
  }

  printf("Done.\n");
  ClientList_print(test);
  printf("Removing elements with id 1...");
  ClientList_detach(test, u1);
  ClientListItem* remove1 = ClientList_findByID(test, 1);
  if (remove1 != NULL){
    printf("ERROR IN DETACH \n");
    flag = -1;
  }
  printf("Done.\n");
  ClientList_print(test);
  printf("Readding elements previously removed...");
  ClientList_insert(test, u1);
  ClientListItem* readd1 = ClientList_findByID(test, 1);
  if (readd1 == NULL) {
    printf("ERROR IN READD \n");
    flag = -1;
  }
  ClientList_insert(test, u2);
  ClientListItem* readd2 = ClientList_findByID(test, 2);
  if (readd2 == NULL) {
    printf("ERROR IN READD \n");
    flag = -1;
  }
  printf("Done.\n");
  printf("Destroying resources for good...");
  ClientList_destroy(test);
  printf("Done.\n");
  fflush(stdout);
  return flag;
}
