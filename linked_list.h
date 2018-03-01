#pragma once
#include "image.h"
#include <netinet/in.h>
#include <time.h>

typedef struct ListItem {
  struct ListItem* prev;
  struct ListItem* next;
  float rotational_force;
  float translational_force;
  float x;
  float y;
  float theta;
  Image * v_texture;
  int id;
  struct sockaddr_in user_addr;
  time_t current_time;
  int isAddrReady;
} ListItem;

typedef struct ListHead {
  ListItem* first;
  ListItem* last;
  int size;
} ListHead;

void List_init(ListHead* head);
ListItem* List_find_by_id(ListHead* head, int id);
ListItem* List_find(ListHead* head, ListItem* item);
ListItem* List_insert(ListHead* head, ListItem* previous, ListItem* item);
ListItem* List_detach(ListHead* head, ListItem* item);
void List_destroy(ListHead* head);
