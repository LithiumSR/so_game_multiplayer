#pragma once
#include <netinet/in.h>
#include <time.h>
#include "../av_framework/image.h"
#include "vehicle.h"
#include "../common/common.h"
typedef struct ClientListItem {
  struct ClientListItem* next;
  int id;
  float x, y, theta, prev_x, prev_y, x_shift, y_shift;
  struct sockaddr_in user_addr_tcp,user_addr_udp;
  struct timeval last_update_time,creation_time,world_update_time;
  char is_udp_addr_ready;
  int afk_counter;
  char inside_world;
  char inside_chat;
  char username[USERNAME_LEN];
  Vehicle* vehicle;
  Image* v_texture;
  float rotational_force, translational_force;
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
