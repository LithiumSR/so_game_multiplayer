#pragma once
#include "image.h"
#include <netinet/in.h>
#include <time.h>
#include "vehicle.h"
typedef struct ClientListItem {
  struct ClientListItem* next;
  int id;
  struct sockaddr_in user_addr;
  time_t last_update_time;
  time_t creation_time;
  char isAddrReady;
  int afk_counter;
  char forceRefresh;
  char insideWorld;
  Vehicle* vehicle;
  Image * v_texture;
} ClientListItem;

typedef struct ClientListHead {
  ClientListItem* first;
  int size;
} ClientListHead;

void ClientList_init(ClientListHead* head);
ClientListItem* ClientList_find_by_id(ClientListHead* head, int id);
ClientListItem* ClientList_find(ClientListHead* head, ClientListItem* item);
ClientListItem* ClientList_insert(ClientListHead* head, ClientListItem* item);
ClientListItem* ClientList_detach(ClientListHead* head, ClientListItem* item);
void ClientList_destroy(ClientListHead* head);
void ClientList_print(ClientListHead* users);
