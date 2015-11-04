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

#define MAXCLIENTS 15

#define IP "127.0.0.1"
#define PORT 8080

struct PACKET {
	int option;
	char nickname[NICKLENGHT];
	char buffer[MESSAGE_SIZE];
};
struct CLIENT_THREAD {
    pthread_t thread_ID; // thread's pointer
    int sockfd; // socket file descriptor
    char nickname[NICKLENGHT]; // client's alias
    int roomID; // user's current room
};
struct CLIENT_OBJ {
    struct CLIENT_THREAD clientThread;
    struct CLIENT_OBJ *next;
};
struct CLIENTS_LIST {
    struct CLIENT_OBJ *head, *tail;
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
};

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

int sockfd, newfd;
struct CLIENT_THREAD thread_info[MAXCLIENTS];
struct CLIENTS_LIST client_list;
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
            struct CLIENT_THREAD clientThread;
	    
            clientThread.sockfd = newfd;
            clientThread.roomID = globalRoom->roomID; // global room
            strcpy(clientThread.nickname, "Undefined");
            pthread_mutex_lock(&clientlist_mutex);
	    
            list_insert(&client_list, &clientThread);
            pthread_mutex_unlock(&clientlist_mutex);
            pthread_create(&clientThread.thread_ID, NULL, client_handler, (void *)&clientThread);
        }
    }
    return 0;
}

void *client_handler(void *fd) {
    struct CLIENT_THREAD clientThread = *(struct CLIENT_THREAD *)fd;
    struct PACKET packet;
    struct CLIENT_OBJ *curr;
    struct ROOM *aux;
    int bytes, sent;

    while(1) {
        bytes = recv(clientThread.sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        if(!bytes) {
            fprintf(stderr, "Connection lost from [%d] %s...\n", clientThread.sockfd, clientThread.nickname);
            pthread_mutex_lock(&clientlist_mutex);
            list_delete(&client_list, &clientThread);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }

        printf("[%d] %i %s %s\n", clientThread.sockfd, packet.option, clientThread.nickname, packet.buffer);
        
        if(packet.option ==  1) {
            printf("Set alias to %s\n", packet.buffer);
            pthread_mutex_lock(&clientlist_mutex);
            for(curr = client_list.head; curr != NULL; curr = curr->next) {
                if(compare(&curr->clientThread, &clientThread) == 0) {
                    strcpy(curr->clientThread.nickname, packet.buffer);
                    strcpy(clientThread.nickname, packet.buffer);
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
		printf("%s Thread.roomID: %i curr->Thread.roomID %i\n", clientThread.nickname, clientThread.roomID, curr->clientThread.roomID);
		if (clientThread.roomID != curr->clientThread.roomID || clientThread.roomID == 0)  continue;
                if (!compare(&curr->clientThread, &clientThread)) continue; 
                spacket.option = 2;
                strcpy(spacket.nickname, clientThread.nickname);
                strcpy(spacket.buffer, packet.buffer);
                sent = send(curr->clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
            }
            pthread_mutex_unlock(&clientlist_mutex);
        }
        else if(packet.option == 3) { // CREATE ROOM
            puts("Creating new room");
            printf("room name %s", packet.buffer);
            struct ROOM* newRoom = (struct ROOM*) malloc(sizeof(struct ROOM));
            strcpy(newRoom->roomName, packet.buffer);
            
	    // Se a sala existe envia pacote informando o problema caso contrario faz o resto e envia sucesso
	    if (roomExists(&room_list, packet.buffer)){
		struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));		
		strcpy(spacket.buffer, "Nao foi possivel criar a sala. Ja existe uma sala com esse nome.");
		strcpy(spacket.nickname, "SERVER");
		sent = send(clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
	    }else{
	    	    pthread_mutex_lock(&roomList_mutex);
		    newRoom->roomID = ++roomCounter;
		    roomListInsert(&room_list, newRoom);
		    pthread_mutex_unlock(&roomList_mutex);
		    	
	    	    pthread_mutex_lock(&clientlist_mutex);
		    // Atualiza nova roomID na lista de clientes 
		    for(curr = client_list.head; curr != NULL; curr = curr->next)
		        if(compare(&curr->clientThread, &clientThread) == 0) {
		            curr->clientThread.roomID = newRoom->roomID;
		            clientThread.roomID = newRoom->roomID;
		            break;
		        }
		    pthread_mutex_unlock(&clientlist_mutex);
	    }
	}
        else if(packet.option == 4) { // LIST ROOMS
		struct PACKET spacket;	 
		memset(&spacket, 0, sizeof(struct PACKET));       
		puts("Listing rooms");
		char msg[MESSAGE_SIZE];
		strcpy(msg, "\nLista de salas:");
		if (room_list.head->next == NULL)
			strcpy(msg, "Nenhuma sala criada. Utilize o comando /create para criar uma sala.");
		else
        		for(aux = room_list.head->next; aux != NULL; aux = aux->next){
				 strcat(msg, "\n");
				strcat(msg, aux->roomName);
			}
		strcpy(spacket.buffer, msg);
		strcpy(spacket.nickname, "SERVER");
		printf("%s\n", msg);
		sent = send(clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
        }

        else if(packet.option == 5){ // JOIN ROOM
            int oldRoom = -1, exitRoom = 0, idRoom = 0;
	    
            if (strcmp(packet.buffer,"noRoom") == 0){ 
		exitRoom = 1;
		oldRoom = clientThread.roomID;
	    }
            
            // Se a sala existe envia pacote informando o problema caso contrario faz o resto e envia sucesso
	    if ((!roomExists(&room_list, packet.buffer)) && exitRoom == 0){
		struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));		
	    	strcpy(spacket.buffer, "Nao foi possivel se conectar. Nao existe nenhuma sala com esse nome.");
		strcpy(spacket.nickname, "SERVER");
		sent = send(clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
	    }else{

		    for(aux = room_list.head; aux != NULL; aux = aux->next)
			if(strcmp(aux->roomName, packet.buffer) == 0)
				idRoom = aux->roomID;

	    	    pthread_mutex_lock(&clientlist_mutex);
		    // Atualiza nova roomID na lista de clientes 
		    for(curr = client_list.head; curr != NULL; curr = curr->next){
		        if(compare(&curr->clientThread, &clientThread) == 0) {
		            curr->clientThread.roomID = idRoom;
		            clientThread.roomID = idRoom;
		            break;
		        }
		    }
		    pthread_mutex_unlock(&clientlist_mutex);

		    // Avisa os outros usuarios da sala da saida ou entrada de um participante
		    struct PACKET spacket;
		    memset(&spacket, 0, sizeof(struct PACKET));
		    strcpy(spacket.nickname, "SERVER");
		    for(curr = client_list.head; curr != NULL; curr = curr->next) {
			if (!compare(&curr->clientThread, &clientThread)) continue; // send to all others
		        if (clientThread.roomID == curr->clientThread.roomID && clientThread.roomID != 0){ 
				char msg[MESSAGE_SIZE];			
				strcpy(msg, clientThread.nickname);
		                strcat(msg, " entrou na sala.");
		            	strcpy(spacket.buffer, msg);
		            	sent = send(curr->clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
			}else if (exitRoom && curr->clientThread.roomID == oldRoom) { 
		            	char msg[MESSAGE_SIZE];
		            	strcpy(msg, clientThread.nickname);
			    	strcat(msg, " saiu da sala.");
		            	strcpy(spacket.buffer, msg);
		            	sent = send(curr->clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET), 0);
		        }
		    }
	    } 
        }  
        
        else if(packet.option == 10) { // QUIT FROM SERVER
            printf("[%d] %s has disconnected...\n", clientThread.sockfd, clientThread.nickname);
            pthread_mutex_lock(&clientlist_mutex);
            list_delete(&client_list, &clientThread);
            pthread_mutex_unlock(&clientlist_mutex);
            break;
        }
        else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", clientThread.sockfd, clientThread.nickname);
        }
    }
 
    close(clientThread.sockfd);
 
    return NULL;
}
