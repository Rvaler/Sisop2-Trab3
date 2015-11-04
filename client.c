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

#define NICKLENGHT 32
#define MESSAGE_SIZE 256
#define ERROR -1

// Tipos de pacote
#define CHANGENICK 1
#define MESSAGE 2
#define CREATE 3
#define LIST 4
#define JOIN 5
#define LEAVE 5
#define QUIT 6

int isConnected = 0;

char userInput[256];
int socket_desc;
int port;
struct hostent *server;
pthread_t receiverThread, senderThread;
char *helpMessage =                 ("\n/nickname NovoNick - Para trocar de nick"
				     "\n/list              - Para ver todas as salas"
				     "\n/create NomeDaSala - Para criar uma nova sala"
				     "\n/join NomeDaSala   - Para se conectar a uma sala existente"
				     "\n/leave             - Para deixar uma sala"
				     "\n/quit              - Para se desconectar"
				     "\n/help              - Ajuda");

struct PACKET {
	int type;
	char nickname[NICKLENGHT];
	char buffer[MESSAGE_SIZE];
};

int connectToServer(int argc, char *argv[]);
void *messageReceiver(void *arg);
void *messageSender(void *arg);
void initUser();

int main(int argc , char *argv[])
{
	
	if(verifyInput(argc, argv) < 0)
		return ERROR;

	if(connectToServer(argc, argv) == ERROR){
		puts("Error in connection");
		return ERROR;
	}

	pthread_create(&senderThread, NULL, messageSender, NULL);
	pthread_create(&receiverThread, NULL, messageReceiver, NULL);

	pthread_join(senderThread, NULL);
	pthread_join(receiverThread, NULL);

	close(socket_desc);

	return 0;
}

void initUser(){
	char *nickPointer, nickname[NICKLENGHT];
	puts("Connected. \nEscolha um nickname: "); 
	fgets(nickname, NICKLENGHT,  stdin);
	if ((nickPointer=strchr(nickname, '\n')) != NULL)
    		*nickPointer = '\0';
	isConnected = 1;
	struct PACKET packet;
	memset(&packet, 0, sizeof(struct PACKET));
	if (strcmp(nickname, "") != 0) {
		strcpy(packet.buffer, nickname);
		packet.type = CHANGENICK;
		int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
	}else
		puts("Entrada invalida. Nickname nao modificado. Uilize o comando /nickname para alterar.");
	puts(helpMessage);
}

int verifyInput(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Error - Missing arguments!\nUse:./client hostname portNumber\n");
		return ERROR;
	}
}

int connectToServer(int argc, char *argv[]) {

	struct sockaddr_in serv_addr;
	port = atoi(argv[2]);

	// gerando endereços de acordo com a entrada do host indicado pelo usuario
	if((server = gethostbyname(argv[1])) == NULL) {
		puts("Error - gethostbyname()");
		return ERROR;	
	}

	// criação do socket 
	if((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Could not create socket");
		return ERROR;	
	}

	// configuração inicial do descritor
	serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons( port );
	memset(&(serv_addr.sin_zero), 0, 8);

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&serv_addr , sizeof(serv_addr)) < 0) {
		puts("connect error");
		return ERROR;
	} else {
		initUser();
		return socket_desc;	
	}
}

void *messageReceiver(void *arg){
	int receivedMessage;
	struct PACKET receivedPacket;
	
	while(isConnected) {
		
		receivedMessage = recv(socket_desc, (void *)&receivedPacket, sizeof(struct PACKET) , 0);
		if(!receivedMessage) {
			puts("Sem conexao com o servidor");
			isConnected = 0;
			pthread_exit(0);
		}else if(receivedMessage > 0)
			printf("[%s]: %s \n", receivedPacket.nickname, receivedPacket.buffer);	
		memset(&receivedPacket, 0, sizeof(struct PACKET));
	}
	pthread_exit(0);
}

void *messageSender(void *arg){
	struct PACKET packet;
	memset(&packet, 0, sizeof(struct PACKET));

	while(isConnected && fgets(userInput, MESSAGE_SIZE, stdin)) {
		char *pos;
		if ((pos=strchr(userInput, '\n')) != NULL)
    			*pos = '\0';

		if(strncmp(userInput, "/nickname", 9) == 0){
			char *nickPointer = strtok(userInput, " ");
			nickPointer = strtok(0, "\0");
			if (nickPointer != NULL) {
				strcpy(packet.buffer, nickPointer);
				packet.type = CHANGENICK;
				int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
			}else
				puts("Entrada invalida. Nickname nao modificado.");
		}else if(strncmp(userInput, "/create", 7) == 0){
			char *roomName = strtok(userInput, " ");
			roomName = strtok(0, "\0");
			if (roomName != NULL) {
				strcpy(packet.buffer, roomName);
				packet.type = CREATE;
				int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
			}else
				puts("Entrada invalida. Nova sala nao foi criada.");
		}else if(strncmp(userInput, "/list", 5) == 0) {
			packet.type = LIST;
			int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
		}else if(strncmp(userInput, "/join", 5) == 0) {
			char *roomName = strtok(userInput, " ");
			roomName = strtok(0, "\0");
			if (roomName != NULL) {
				strcpy(packet.buffer, roomName);
				packet.type = JOIN;
				int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
			}else
				puts("Entrada invalida. Nao foi possivel entrar na sala.");
		}else if(strncmp(userInput, "/leave", 6) == 0){
			packet.type = LEAVE;
			strcpy(packet.buffer, "noRoom");
			int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
		}else if(strncmp(userInput, "/help", 5) == 0){
			puts(helpMessage);
		}else if(strncmp(userInput, "/quit", 5) == 0){
			puts("Desconectando...");
			packet.type = QUIT;
			int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
			isConnected = 0;
		}else{
			char *msg = userInput;
			strcpy(packet.buffer, msg);
			packet.type = MESSAGE;
			int sent = send(socket_desc, (void *)&packet, sizeof(struct PACKET), 0);
		}
		
	}
	pthread_exit(0);
}
