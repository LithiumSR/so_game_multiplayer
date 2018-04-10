#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../game_framework/so_game_protocol.h"
#include "../game_framework/client_list.h"
#include "../common/common.h"
int main(int argc, char const *argv[]) {
    debug_print("Starting client list tests...");
    ClientListHead* test=(ClientListHead*)malloc(sizeof(ClientListHead));
    ClientList_print(test);
    ClientListItem* u1=(ClientListItem*)malloc(sizeof(ClientListItem));
    u1->id=1;
    u1->v_texture=NULL;
    debug_print("Add element with id 1...");
    ClientList_insert(test,u1);
    ClientList_print(test);
    ClientListItem* u2=(ClientListItem*)malloc(sizeof(ClientListItem));
    u2->id=2;
    u2->v_texture=NULL;
    debug_print("Add element with id 2...");
    ClientList_insert(test,u2);
    ClientList_print(test);
    ClientListItem* cil=ClientList_find_by_id(test,2);
    debug_print("Finding and comparing elements with id 2...");
    if(cil==NULL) debug_print("ERROR IN DETACH \n");
    if(cil==u2) debug_print("OK \n");
    ClientListItem* cil2=ClientList_find_by_id(test,1);
    debug_print("Finding and omparing elements with id 1...");
    if(cil2==NULL) debug_print("ERROR IN DETACH \n");
    if(cil2==u1) debug_print("OK \n ");
    debug_print("Removing elements with id 2... \n");
    ClientList_detach(test,cil2);
    ClientList_print(test);
    debug_print("Removing elements with id 1... \n");
    ClientList_detach(test,cil);
    ClientList_print(test);
    debug_print("Readding elements previously removed... \n");
    ClientList_insert(test,u1);
    ClientList_insert(test,u2);
    debug_print("Destroying resources for good... \n");
    ClientList_destroy(test);
    fflush(stdout);
}
