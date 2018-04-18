#pragma once
#include <netinet/in.h>
#include <time.h>
#include "../av_framework/image.h"

typedef struct ClientListItem {
  struct ClientListItem *next;
  float rotational_force;
  float translational_force;
  float x, y, theta;
  float prev_x, prev_y;
  float x_shift, y_shift;
  Image *v_texture;
  int id;
  struct sockaddr_in user_addr_udp, user_addr_tcp;
  struct timeval last_update_time, creation_time;
  int is_udp_addr_ready;
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
