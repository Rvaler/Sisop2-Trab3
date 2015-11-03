#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct ROOM{
    int roomID;
    char roomName[255];
    struct ROOM *next;
};
struct ROOMLIST {
    struct ROOM *start, *end;
    int size;
};
int roomListInsert(struct ROOMLIST *ll, struct ROOM *room) {
	room->next = NULL;
	if (ll->start == NULL){
		ll->start = room;
		ll->end = room;
	}else{
		ll->end->next = room;
		ll->end = room;
	}
	return 0;
}
void roomListInit(struct ROOMLIST *ll){
    ll->start = ll->end = NULL;
    ll->size = 0;
}
struct ROOM* newRoom(char *name, int id){
	struct ROOM* room = (struct ROOM*) malloc(sizeof(struct ROOM));
	room->roomID = id;
	strcpy(room->roomName, name);
	room->next = NULL;
	return room;
}
int main(){
	struct ROOMLIST room_list;
	roomListInit(&room_list);
	struct ROOM *sala1, *sala2, *sala3;
	sala1 = newRoom("aa", 10);
	sala2 = newRoom("bb", 11);
	sala3 = newRoom("cc", 12);
	roomListInsert(&room_list, sala1);
	roomListInsert(&room_list, sala2);
	roomListInsert(&room_list, sala3);
    	printf("%d %s\n", room_list.start->roomID, room_list.start->roomName);
	printf("%d %s\n", room_list.start->next->roomID, room_list.start->roomName);
	printf("%d %s\n", room_list.start->next->next->roomID, room_list.start->roomName);
	printf("%d %s\n", room_list.end->roomID, room_list.start->roomName);
}
