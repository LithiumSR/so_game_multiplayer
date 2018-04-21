#pragma once
#include <netinet/in.h>
#include <time.h>
#include "../av_framework/image.h"
#include "vehicle.h"
typedef struct MessageListItem {
  struct MessageListItem* next;
  int id;
  char text[256];
  char sender[32];
  time_t time;
} MessageListItem;

typedef struct MessageListHead {
  MessageListItem* first;
  MessageListItem* last;
  int size;
} MessageListHead;

void MessageList_init(MessageListHead* head);
MessageListItem* MessageList_find(MessageListHead* head, MessageListItem* item);
MessageListItem* MessageList_insert(MessageListHead* head,
                                    MessageListItem* item);
MessageListItem* MessageList_detach(MessageListHead* head,
                                    MessageListItem* item);
void MessageList_destroy(MessageListHead* head);
void MessageList_print(MessageListHead* users);
void MessageList_removeAll(MessageListHead* users);