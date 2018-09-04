#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../game_framework/message_list.h"
#include "../game_framework/protogame_protocol.h"

#if SERVER_SIDE_POSITION_CHECK == 1
#define _USE_SERVER_SIDE_FOG_
#endif

MessageListItem* findById(MessageListHead* head, int id){
  if (head == NULL) return NULL;
  MessageListItem* item = head->first;
  while(item!=NULL){
    if (item->id == id) return item;
    item = item->next;
  }
  return NULL;
}

int main(int argc, char const* argv[]) {
  char flag = 0;
  printf("Creating message list...");
  MessageListHead* list = (MessageListHead*)malloc(sizeof(MessageListHead));
  MessageList_init(list);
  printf("Done.\n");
  printf("Adding message 1 to list...");
  MessageListItem* el = (MessageListItem*)malloc(sizeof(MessageListItem));
  el->id = 10;
  char* sender1 = "username_test 1";
  char* text1 = "Hello World 1";
  strncpy(el->sender, sender1, USERNAME_LEN);
  strncpy(el->text, text1, TEXT_LEN);
  time(&el->time);
  el->type = Hello;
  printf("Done.\n");
  MessageList_insert(list, el);
  if (findById(list,10) == NULL) {
    printf("ERROR IN ADD\n");
    flag = -1;
  }
  MessageList_print(list);
  printf("Adding message 2 to list...");
  MessageListItem* el2 = (MessageListItem*)malloc(sizeof(MessageListItem));
  el2->id = 11;
  char* sender2 = "username_test 2";
  char* text2 = "Hello World 2";
  strncpy(el2->sender, sender2, USERNAME_LEN);
  strncpy(el2->text, text2, TEXT_LEN);
  time(&el2->time);
  el2->type = Hello;
  MessageList_insert(list, el2);
  if (findById(list,11) == NULL) {
    printf("ERROR IN ADD\n");
    flag = -1;
  }
  printf("Done.\n");
  MessageList_print(list);
  printf("Removing all messages...");
  MessageList_removeAll(list);
  if (findById(list,10) != NULL || findById(list,11)!= NULL) {
    printf("ERROR IN REMOVEALL\n");
    flag = -1;
  }
  printf("Done.\n");
  MessageList_print(list);
  return flag;
}
