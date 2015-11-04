#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>

#define ROOMNAMELENGHT 64
#define NICKLENGHT 32
#define MESSAGE_SIZE 256
#define ERROR -1

#define MAXCLIENTS 8

#define IP "127.0.0.1"
#define PORT 8080

struct PACKET {
	int option;
	char nickname[NICKLENGHT];
	char buffer[MESSAGE_SIZE];
};
struct THREADINFO {
    pthread_t thread_ID; // thread's pointer
    int sockfd; // socket file descriptor
    char nickname[NICKLENGHT]; // client's alias
    int roomID; // user's current room
};
struct LLNODE {
    struct THREADINFO threadinfo;
    struct LLNODE *next;
};
struct LLIST {
    struct LLNODE *head, *tail;
    int size;
};
struct ROOM
{
    int roomID;
    char roomName[ROOMNAMELENGHT];
    struct ROOM *next;
};
struct ROOMLIST {
    struct ROOM *head, *tail;
    int size;
};

struct ROOM* newRoom(char *name, int id){
	struct ROOM* room = (struct ROOM*) malloc(sizeof(struct ROOM));
	room->roomID = id;
	strcpy(room->roomName, name);
	room->next = NULL;
	return room;
}

void roomListInit(struct ROOMLIST *ll){
    ll->head = ll->tail = NULL;
    ll->size = 0;
}

int roomListInsert(struct ROOMLIST *ll, struct ROOM *room) {
    room->next = NULL;
    if (ll->head == NULL){
        ll->head = room;
        ll->tail = room;
    }else{
        ll->tail->next = room;
        ll->tail = room;
    }
    return 0;
}

int roomListDelete(struct ROOMLIST *ll, struct ROOM *room) {
    struct ROOM *curr, *temp;
    if(ll->head == NULL) return ERROR;
    if(compareRoom(room, &ll->head) == 0) {
        temp = ll->head;
        ll->head = ll->head->next;
        if(ll->head == NULL) ll->tail = ll->head;
        free(temp);
        ll->size--;
        return 0;
    }
    for(curr = ll->head; curr->next != NULL; curr = curr->next) {
        if(compareRoom(room, &curr->next) == 0) {
            temp = curr->next;
            if(temp == ll->tail) ll->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            ll->size--;
            return 0;
        }
    }
    return ERROR;
}

int compareRoom(struct ROOM *a, struct ROOM *b) {
    return a->roomID - b->roomID;
}

int compare(struct THREADINFO *a, struct THREADINFO *b) {
    return a->sockfd - b->sockfd;
}
 
void list_init(struct LLIST *ll) {
    ll->head = ll->tail = NULL;
    ll->size = 0;
}

int list_insert(struct LLIST *ll, struct THREADINFO *thr_info) {
    if(ll->size == MAXCLIENTS) return ERROR;
    if(ll->head == NULL) {
        ll->head = (struct LLNODE *)malloc(sizeof(struct LLNODE));
        ll->head->threadinfo = *thr_info;
        ll->head->next = NULL;
        ll->tail = ll->head;
    }
    else {
        ll->tail->next = (struct LLNODE *)malloc(sizeof(struct LLNODE));
        ll->tail->next->threadinfo = *thr_info;
        ll->tail->next->next = NULL;
        ll->tail = ll->tail->next;
    }
    ll->size++;
    return 0;
}
 
int list_delete(struct LLIST *ll, struct THREADINFO *thr_info) {
    struct LLNODE *curr, *temp;
    if(ll->head == NULL) return ERROR;
    if(compare(thr_info, &ll->head->threadinfo) == 0) {
        temp = ll->head;
        ll->head = ll->head->next;
        if(ll->head == NULL) ll->tail = ll->head;
        free(temp);
        ll->size--;
        return 0;
    }
    for(curr = ll->head; curr->next != NULL; curr = curr->next) {
        if(compare(thr_info, &curr->next->threadinfo) == 0) {
            temp = curr->next;
            if(temp == ll->tail) ll->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            ll->size--;
            return 0;
        }
    }
    return ERROR;
}

void printRoomList(struct ROOMLIST *list) {
    struct ROOM *current;
    for(current = list->head; current != NULL; current = current->next) {
        printf("%i - %s\n", current->roomID, current->roomName);
    }
}

int sockfd, newfd;
struct THREADINFO thread_info[MAXCLIENTS];
struct LLIST client_list;
pthread_mutex_t clientlist_mutex;

int roomCounter = 0;
struct ROOMLIST room_list;
pthread_mutex_t roomList_mutex;

void *client_handler(void *fd);

int main(int argc, char **argv) {

    int sin_size;
    struct sockaddr_in serv_addr, client_addr;
    pthread_t interrupt;
 
    list_init(&client_list);
    roomListInit(&room_list);
 
    pthread_mutex_init(&clientlist_mutex, NULL);
    pthread_mutex_init(&roomList_mutex, NULL);
 
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        puts("Error on socket creation");
        return ERROR;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    memset(&(serv_addr.sin_zero), 0, 8);
   
    if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == ERROR) {
        puts("Error on binding server");
        return ERROR;
    }
    
    if(listen(sockfd, MAXCLIENTS) == ERROR) {
        puts("Error on listening");
        return ERROR;
    }

    struct ROOM *globalRoom;
    globalRoom = newRoom("noRoom", 0);
    roomListInsert(&room_list, globalRoom);

    printf("Starting listener...\n");
    while(1) {
        sin_size = sizeof(struct sockaddr_in);
        if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&sin_size)) == ERROR) {
            puts("Error on accepting connection");
            return ERROR;
        } else {
            if(client_list.size == MAXCLIENTS) {
                puts("Connection full, connection rejected");
                continue;
            }
            puts("Connection requested received...");
            struct THREADINFO threadinfo;
	    
            threadinfo.sockfd = newfd;
            threadinfo.roomID = globalRoom->roomID; // global room
            strcpy(threadinfo.nickname, "Undefined");
            pthread_mutex_lock(&clientlist_mutex);
	    
            list_insert(&client_list, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            pthread_create(&threadinfo.thread_ID, NULL, client_handler, (void *)&threadinfo);
        }
    }
    return 0;
}

void *client_handler(void *fd) {
    struct THREADINFO threadinfo = *(struct THREADINFO *)fd;
    struct PACKET packet;
    struct LLNODE *curr;
    struct ROOM *aux;
    int bytes, sent;
    while(1) {
        bytes = recv(threadinfo.sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        if(!bytes) {
            fprintf(stderr, "Connection lost from [%d] %s...\n", threadinfo.sockfd, threadinfo.nickname);
            pthread_mutex_lock(&clientlist_mutex);
            list_delete(&client_list, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }
        printf("[%d] %i %s %s\n", threadinfo.sockfd, packet.option, threadinfo.nickname, packet.buffer);
        
        if(packet.option ==  1) {
            printf("Set alias to %s\n", packet.buffer);
            pthread_mutex_lock(&clientlist_mutex);
            for(curr = client_list.head; curr != NULL; curr = curr->next) {
                if(compare(&curr->threadinfo, &threadinfo) == 0) {
                    strcpy(curr->threadinfo.nickname, packet.buffer);
                    strcpy(threadinfo.nickname, packet.buffer);
                    break;
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        } 
        
        if(packet.option == 2) { // SEND
            pthread_mutex_lock(&clientlist_mutex);
            for(curr = client_list.head; curr != NULL; curr = curr->next) {
                struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));
		        printf("%s threadinfo.roomID: %i curr->threadinfo.roomID %i\n", threadinfo.nickname, threadinfo.roomID, curr->threadinfo.roomID);
		        if (threadinfo.roomID != curr->threadinfo.roomID || threadinfo.roomID == 0)  continue;
                if (!compare(&curr->threadinfo, &threadinfo)) continue; 
                spacket.option = 2;
                strcpy(spacket.nickname, threadinfo.nickname);
                strcpy(spacket.buffer, packet.buffer);
                sent = send(curr->threadinfo.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }
        else if(packet.option == 3) { // CREATE ROOM
            puts("Creating new room");
            printf("room name %s", packet.buffer);
            struct ROOM* newRoom = (struct ROOM*) malloc(sizeof(struct ROOM));
            strcpy(newRoom->roomName, packet.buffer);
            
    	    pthread_mutex_lock(&roomList_mutex);
            newRoom->roomID = ++roomCounter;
            roomListInsert(&room_list, newRoom);
            pthread_mutex_unlock(&roomList_mutex);
            	
    	    pthread_mutex_lock(&clientlist_mutex);

	    // Atualiza nova roomID na lista de clientes 
            for(curr = client_list.head; curr != NULL; curr = curr->next){ 
                if(compare(&curr->threadinfo, &threadinfo) == 0) {
                    curr->threadinfo.roomID = newRoom->roomID;
                    threadinfo.roomID = newRoom->roomID;
                    break;
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
	    }

        else if(packet.option == 4) { // LIST ROOMS
            puts("Listing rooms");
            printRoomList(&room_list);
        }

        else if(packet.option == 5){ // JOIN ROOM
            int oldRoom = -1, exitRoom = 0, idRoom = 0;
	    
            if (strcmp(packet.buffer,"noRoom") == 0){ 
		exitRoom = 1;
		oldRoom = threadinfo.roomID;
	    }
	    for(aux = room_list.head; aux != NULL; aux = aux->next)
		if(strcmp(aux->roomName, packet.buffer) == 0)
        		idRoom = aux->roomID;

    	    pthread_mutex_lock(&clientlist_mutex);
	    // Atualiza nova roomID na lista de clientes 
            for(curr = client_list.head; curr != NULL; curr = curr->next){
                if(compare(&curr->threadinfo, &threadinfo) == 0) {
                    curr->threadinfo.roomID = idRoom;
                    threadinfo.roomID = idRoom;
                    break;
                }
            }
	    struct PACKET spacket;
            memset(&spacket, 0, sizeof(struct PACKET));
	    strcpy(spacket.nickname, "SERVER");
            for(curr = client_list.head; curr != NULL; curr = curr->next) {
		if (!compare(&curr->threadinfo, &threadinfo)) continue; // send to all others
                if (threadinfo.roomID == curr->threadinfo.roomID && threadinfo.roomID != 0){ 
			char msg[MESSAGE_SIZE];			
			strcpy(msg, threadinfo.nickname);
                        strcat(msg, " entered the room");
                    	strcpy(spacket.buffer, msg);
                    	sent = send(curr->threadinfo.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
		}else if (exitRoom && curr->threadinfo.roomID == oldRoom) { 
                    	char msg[MESSAGE_SIZE];
                    	strcpy(msg, threadinfo.nickname);
		    	strcat(msg, " left the room");
                    	strcpy(spacket.buffer, msg);
                    	sent = send(curr->threadinfo.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
                }
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }  
        
        else if(packet.option == 10) { // QUIT FROM SERVER
            printf("[%d] %s has disconnected...\n", threadinfo.sockfd, threadinfo.nickname);
            pthread_mutex_lock(&clientlist_mutex);
            list_delete(&client_list, &threadinfo);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }
        else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", threadinfo.sockfd, threadinfo.nickname);
        }
    }
 
    close(threadinfo.sockfd);
 
    return NULL;
}
