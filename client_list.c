#include "client_list.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void ClientList_init(ClientListHead* head) {
  head->first=0;
  head->size=0;
}

ClientListItem* ClientList_find_by_id(ClientListHead* head, int id){
    if(head==NULL) return NULL;
	ClientListItem* tmp=head->first;
	while(tmp!=NULL){
		if(tmp->id==id) return tmp;
		else tmp=tmp->next;
	}
	return NULL;
}

ClientListItem* ClientList_insert(ClientListHead* head, ClientListItem* item) {
    if(head==NULL) return NULL;
    ClientListItem* client=head->first;
    item->next=client;
    head->first=item;
    head->size++;
    return item;
}

ClientListItem* ClientList_detach(ClientListHead* head, ClientListItem* item) {
    if(head==NULL) return NULL;
    ClientListItem* client=head->first;
    if(client==item) {
        head->first=client->next;
        head->size--;
        return client;
    }
    while(client!=NULL){
        if(client->next==item) {
            client->next=item->next;
            head->size--;
            return item;
        }
        else client=client->next;
    }
    return NULL;
}

void ClientList_destroy(ClientListHead* users){

    ClientListItem* user = users->first;
    int i=0;
    while(user!=NULL){
		ClientListItem* tmp=ClientList_detach(users, user);
		user=user->next;
        if (tmp->v_texture!=NULL) Image_free(tmp->v_texture);
		close(tmp->id);
        free(tmp);
        i++;
	}
    free(users);
}

void ClientList_print(ClientListHead* users){

    ClientListItem* user = users->first;
    int i=0;
    printf("List elements: [");
    while(i<users->size){
		if (user!=NULL) {
            printf("%d,",user->id);
            user=user->next;
            i++;
        }
        else break;
	}
    printf("]\n");
}




