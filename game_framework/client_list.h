#pragma once
#include "../av_framework/image.h"
#include <netinet/in.h>
#include <time.h>
#include "vehicle.h"
typedef struct ClientListItem {
  struct ClientListItem* next;
  int id;
  float x,y,theta,prev_x,prev_y,x_shift,y_shift;
  
  struct sockaddr_in user_addr;
  struct timeval last_update_time;
  struct timeval creation_time;
  struct timeval world_update_time;
  char is_addr_ready;
  int afk_counter;
  char inside_world;
  Vehicle* vehicle;
  Image * v_texture;
  float rotational_force,translational_force;
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
