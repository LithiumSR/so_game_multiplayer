#include "message_list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void MessageList_init(MessageListHead* head) {
  head->first = NULL;
  head->last = NULL;
  head->size = 0;
}

MessageListItem* MessageList_insert(MessageListHead* head,
                                    MessageListItem* item) {
  if (head == NULL) return NULL;
  MessageListItem* client = head->last;
  if (client != NULL) client->next = item;
  head->last = item;
  item->next = NULL;
  if (head->first == NULL) head->first = item;
  head->size++;
  return item;
}

MessageListItem* MessageList_detach(MessageListHead* head,
                                    MessageListItem* item) {
  if (head == NULL) return NULL;
  MessageListItem* client = head->first;
  if (client == NULL) return NULL;
  if (client == item) {
    if (client == head->last) head->last = NULL;
    head->first = client->next;
    head->size--;
    return client;
  }
  while (client != NULL) {
    if (client->next == item) {
      if (client->next == head->last) head->last = client;
      client->next = item->next;
      head->size--;
      return item;
    } else
      client = client->next;
  }
  return NULL;
}

void MessageList_destroy(MessageListHead* users) {
  if (users == NULL) return;
  MessageListItem* user = users->first;
  while (user != NULL) {
    MessageListItem* tmp = MessageList_detach(users, user);
    user = user->next;
    free(tmp);
  }
  free(users);
}

void MessageList_removeAll(MessageListHead* users) {
  if (users == NULL) return;
  MessageListItem* user = users->first;
  while (user != NULL) {
    MessageListItem* tmp = MessageList_detach(users, user);
    user = users->first;
    free(tmp);
  }
  users->first = NULL;
  users->last = NULL;
}

MessageListItem* MessageList_addDisconnectMessage(MessageListHead* head,
                                                  ClientListItem* user) {
  if (head == NULL || !user->inside_chat) return NULL;
  MessageListItem* item = (MessageListItem*)malloc(sizeof(MessageListItem));
  item->id = user->id;
  item->type = Goodbye;
  time(&item->time);
  strncpy(item->sender, user->username, USERNAME_LEN);
  MessageList_insert(head, item);
  user->inside_chat = 0;
  return item;
}

void MessageList_print(MessageListHead* messages) {
  if (messages == NULL) return;
  MessageListItem* message = messages->first;
  int i = 0;
  printf("List elements: \n[");
  while (i < messages->size) {
    if (message != NULL) {
      struct tm* info;
      info = localtime(&message->time);
      printf("(id: %d,", message->id);
      printf("sender: %s,", message->sender);
      printf("text: %s,", message->text);
      printf("messageType: %d",message->type);
      if(i==messages->size-1) printf("time: %d:%d)", info->tm_hour, info->tm_min);
      else printf("time: %d:%d),\n", info->tm_hour, info->tm_min);
      message = message->next;
      i++;
    } else
      break;
  }
  printf("]\n");
}