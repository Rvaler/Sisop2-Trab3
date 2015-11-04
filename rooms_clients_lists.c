#include "rooms_clients_lists.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ROOM* newRoom(char *name, int id){
	struct ROOM* room = (struct ROOM*) malloc(sizeof(struct ROOM));
	room->roomID = id;
	strcpy(room->roomName, name);
	room->next = NULL;
	return room;
}

void roomListInit(struct ROOMLIST *list){
    list->head = list->tail = NULL;
}

int roomListInsert(struct ROOMLIST *list, struct ROOM *room) {
    room->next = NULL;
    if (list->head == NULL){
        list->head = room;
        list->tail = room;
    }else{
        list->tail->next = room;
        list->tail = room;
    }
    return 0;
}

int roomListDelete(struct ROOMLIST *list, struct ROOM *room) {
    struct ROOM *curr, *temp;
    if(list->head == NULL) return ERROR;
    if(compareRoom(room, &list->head) == 0) {
        temp = list->head;
        list->head = list->head->next;
        if(list->head == NULL) list->tail = list->head;
        free(temp);
        return 0;
    }
    for(curr = list->head; curr->next != NULL; curr = curr->next) {
        if(compareRoom(room, &curr->next) == 0) {
            temp = curr->next;
            if(temp == list->tail) list->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            return 0;
        }
    }
    return ERROR;
}

int compareRoom(struct ROOM *a, struct ROOM *b) {
    return a->roomID - b->roomID;
}

int roomExists(struct ROOMLIST *list, char *roomName) {
	struct ROOM *current;	
	for(current = list->head; current != NULL; current = current->next) 
		if(strcmp(current->roomName, roomName) == 0)
			return 1;
	return 0;
}

int compare(struct CLIENT_THREAD *a, struct CLIENT_THREAD *b) {
    return a->sockfd - b->sockfd;
}
 
void list_init(struct CLIENTS_LIST *list) {
    list->head = list->tail = NULL;
    list->size = 0;
}

int list_insert(struct CLIENTS_LIST *list, struct CLIENT_THREAD *thr_info) {
    if(list->size == MAXCLIENTS) return ERROR;
    if(list->head == NULL) {
        list->head = (struct CLIENT_OBJ *)malloc(sizeof(struct CLIENT_OBJ));
        list->head->clientThread = *thr_info;
        list->head->next = NULL;
        list->tail = list->head;
    }
    else {
        list->tail->next = (struct CLIENT_OBJ *)malloc(sizeof(struct CLIENT_OBJ));
        list->tail->next->clientThread = *thr_info;
        list->tail->next->next = NULL;
        list->tail = list->tail->next;
    }
    list->size++;
    return 0;
}
 
int list_delete(struct CLIENTS_LIST *list, struct CLIENT_THREAD *thr_info) {
    struct CLIENT_OBJ *curr, *temp;
    if(list->head == NULL) return ERROR;
    if(compare(thr_info, &list->head->clientThread) == 0) {
        temp = list->head;
        list->head = list->head->next;
        if(list->head == NULL) list->tail = list->head;
        free(temp);
        list->size--;
        return 0;
    }
    for(curr = list->head; curr->next != NULL; curr = curr->next) {
        if(compare(thr_info, &curr->next->clientThread) == 0) {
            temp = curr->next;
            if(temp == list->tail) list->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            list->size--;
            return 0;
        }
    }
    return ERROR;
}

void printRoomList(struct ROOMLIST *list) {
    struct ROOM *current;
    for(current = list->head->next; current != NULL; current = current->next) {
        printf("%i - %s\n", current->roomID, current->roomName);
    }
}

