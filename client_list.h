#pragma once
#include <netinet/in.h>
#include <time.h>
#include "image.h"

typedef struct ClientListItem {
  struct ClientListItem *next;
  float rotational_force;
  float translational_force;
  float x;
  float prev_x;
  float y;
  float prev_y;
  float theta;
  float x_shift, y_shift;
  Image *v_texture;
  int id;
  struct sockaddr_in user_addr;
  struct timeval last_update_time, creation_time;
  int is_addr_ready;
  int afk_counter;
  int force_refresh;
} ClientListItem;

typedef struct ClientListHead {
  ClientListItem *first;
  int size;
} ClientListHead;

void ClientList_init(ClientListHead *head);
ClientListItem *ClientList_find_by_id(ClientListHead *head, int id);
ClientListItem *ClientList_find(ClientListHead *head, ClientListItem *item);
ClientListItem *ClientList_insert(ClientListHead *head, ClientListItem *item);
ClientListItem *ClientList_detach(ClientListHead *head, ClientListItem *item);
void ClientList_destroy(ClientListHead *head);
void ClientList_print(ClientListHead *users);
