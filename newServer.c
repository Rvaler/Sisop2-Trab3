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
#include "rooms_clients_lists.c"

struct PACKET {
	int type;
	char nickname[NICKLENGHT];
	char buffer[MESSAGE_SIZE];
};

int roomCounter = 0;
int sockfd, newfd;
struct CLIENT_THREAD thread_info[MAXCLIENTS];
struct CLIENTS_LIST client_list;
struct ROOMLIST room_list;
pthread_mutex_t roomListMutex;
pthread_mutex_t clientListMutex;

void *client_handler(void *fd);

int main(int argc, char **argv) {

    int ssize;
    struct sockaddr_in serv_addr, client_addr;
    pthread_t interrupt;
 
    list_init(&client_list);
    roomListInit(&room_list);
 
    pthread_mutex_init(&clientListMutex, NULL);
    pthread_mutex_init(&roomListMutex, NULL);
 
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
        ssize = sizeof(struct sockaddr_in);
        if((newfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&ssize)) == ERROR) {
            puts("Error on accepting connection");
            return ERROR;
        } else {
            if(client_list.size == MAXCLIENTS) {
                puts("Sala cheia, a conexao nao sera possivel");
                continue;
            }
            puts("Novo usuario conectado");
            struct CLIENT_THREAD clientThread;
	    
            clientThread.sockfd = newfd;
            clientThread.roomID = globalRoom->roomID; // global room
            strcpy(clientThread.nickname, "Undefined");
            pthread_mutex_lock(&clientListMutex);
            list_insert(&client_list, &clientThread);
            pthread_mutex_unlock(&clientListMutex);
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
        bytes = read(clientThread.sockfd, (void *)&packet, sizeof(struct PACKET));
        if(!bytes) {
            printf("Conexao do usuario %s perdida\n", clientThread.nickname);
            pthread_mutex_lock(&clientListMutex);
            list_delete(&client_list, &clientThread);
            pthread_mutex_unlock(&clientListMutex);
            break;
        }
        
        if(packet.type ==  CHANGENICK) {
            printf("Nickame setado para %s\n", packet.buffer);
            pthread_mutex_lock(&clientListMutex);
            for(curr = client_list.head; curr != NULL; curr = curr->next)
                if(compare(&curr->clientThread, &clientThread) == 0) {
                    strcpy(curr->clientThread.nickname, packet.buffer);
                    strcpy(clientThread.nickname, packet.buffer);
                    break;
                }
            pthread_mutex_unlock(&clientListMutex);
        }  
        if(packet.type == MESSAGE) { // SEND
	    printf("id da sala: %d usuario: %s mensagem: %s\n", clientThread.roomID, clientThread.nickname, packet.buffer);
            pthread_mutex_lock(&clientListMutex);
            for(curr = client_list.head; curr != NULL; curr = curr->next) {
                struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));
		if (clientThread.roomID != curr->clientThread.roomID || clientThread.roomID == 0)  continue;
                if (!compare(&curr->clientThread, &clientThread)) continue; 
                strcpy(spacket.nickname, clientThread.nickname);
                strcpy(spacket.buffer, packet.buffer);
                sent = write(curr->clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET));
            }
            pthread_mutex_unlock(&clientListMutex);
        }else if(packet.type == CREATE) { // CREATE ROOM
            puts("Requisicao de criacao de nova sala");      
            struct ROOM* newRoom = (struct ROOM*) malloc(sizeof(struct ROOM));
            strcpy(newRoom->roomName, packet.buffer);  
	    // Verifica se ja existe alguma sala com o nome informado
	    if (roomExists(&room_list, packet.buffer)){
		struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));		
		strcpy(spacket.buffer, "Nao foi possivel criar a sala. Ja existe uma sala com esse nome.");
		strcpy(spacket.nickname, "SERVER");
		sent = write(clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET));
	    }else{
	    	    pthread_mutex_lock(&roomListMutex);
		    newRoom->roomID = ++roomCounter;
		    roomListInsert(&room_list, newRoom);
		    pthread_mutex_unlock(&roomListMutex);
		    printf("Criada nova sala %s\n", packet.buffer);
		    // Atualiza nova roomID na lista de clientes 	
	    	    pthread_mutex_lock(&clientListMutex);
		    for(curr = client_list.head; curr != NULL; curr = curr->next)
		        if(compare(&curr->clientThread, &clientThread) == 0) {
		            curr->clientThread.roomID = newRoom->roomID;
		            clientThread.roomID = newRoom->roomID;
		            break;
		        }
		    pthread_mutex_unlock(&clientListMutex);
	    }
	}else if(packet.type == LIST) { // LIST ROOMS
		puts("Requisicao de listagem de salas");  
		struct PACKET spacket;	 
		memset(&spacket, 0, sizeof(struct PACKET));       
		char msg[MESSAGE_SIZE];
		strcpy(msg, "\nLista de salas:");
		if (room_list.head->next == NULL)
			strcpy(msg, "Nenhuma sala criada. Utilize o comando /create para criar uma sala.");
		else
        		for(aux = room_list.head->next; aux != NULL; aux = aux->next){
				 strcat(msg, "\n");
				strcat(msg, aux->roomName);
			}
		puts(msg);
		strcpy(spacket.buffer, msg);
		strcpy(spacket.nickname, "SERVER");
		sent = write(clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET));
        }else if(packet.type == JOIN){ // JOIN ROOM
	    puts("Requisicao de mudanca de sala"); 
	    int oldRoom = -1, exitRoom = 0, idRoom = 0;
            if (strcmp(packet.buffer,"noRoom") == 0){ 
		exitRoom = 1;
		oldRoom = clientThread.roomID;
	    }
            // Verifica se a sala a qual o usuario esta tentando se conectar existe
	    if ((!roomExists(&room_list, packet.buffer)) && exitRoom == 0){
		puts("Nao foi possivel se conectar. Nao existe nenhuma sala com esse nome."); 		
		struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));		
	    	strcpy(spacket.buffer, "Nao foi possivel se conectar. Nao existe nenhuma sala com esse nome.");
		strcpy(spacket.nickname, "SERVER");
		sent = write(clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET));
	    }else{

		    for(aux = room_list.head; aux != NULL; aux = aux->next)
			if(strcmp(aux->roomName, packet.buffer) == 0)
				idRoom = aux->roomID;
		    
		    // Atualiza nova roomID na lista de clientes
	    	    pthread_mutex_lock(&clientListMutex); 
		    for(curr = client_list.head; curr != NULL; curr = curr->next)
		        if(compare(&curr->clientThread, &clientThread) == 0) {
		            curr->clientThread.roomID = idRoom;
		            clientThread.roomID = idRoom;
		            break;
		        }
		    pthread_mutex_unlock(&clientListMutex);

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
		            	sent = write(curr->clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET));
			}else if (exitRoom && curr->clientThread.roomID == oldRoom) { 
		            	char msg[MESSAGE_SIZE];
		            	strcpy(msg, clientThread.nickname);
			    	strcat(msg, " saiu da sala.");
		            	strcpy(spacket.buffer, msg);
		            	sent = write(curr->clientThread.sockfd, (void *)&spacket, sizeof(struct PACKET));
		        }
		    }
	    } 
        }else if(packet.type == QUIT) { // QUIT FROM SERVER
            printf("%s se desconectou\n", clientThread.nickname);
            pthread_mutex_lock(&clientListMutex);
            list_delete(&client_list, &clientThread);
            pthread_mutex_unlock(&clientListMutex);
            break;
        }/*else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", clientThread.sockfd, clientThread.nickname);
        }*/
    }
 
    close(clientThread.sockfd);
 
    return 0;
}
