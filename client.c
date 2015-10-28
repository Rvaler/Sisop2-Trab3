
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

int isConnected = 0;

char option[256];
int socket_desc;
int port;
struct hostent *server;
pthread_t receiverThread, senderThread;
//char *inAddress = "201.21.215.114";

// put this in .h later
struct PACKET {
	int roomID;
	char nickname[NICKLENGHT];
	char buffer[MESSAGE_SIZE];
};


int connectToServer(int argc, char *argv[]);
void *messageReceiver(void *arg);
void *messageSender(void *arg);

int main(int argc , char *argv[])
{
	
	if(verifyInput(argc, argv) < 0)
		return ERROR;

	if(connectToServer(argc, argv) == -1){
		puts("Error in connection");
		return ERROR;
	}


	
/*
	while(gets(option)){ // TODO: implementar o menu aqui
		if(!strncmp(option, "exit", 4)) {
			puts("entrou");
		}
	}
	*/

	pthread_create(&senderThread, NULL, messageSender, NULL);
	pthread_create(&receiverThread, NULL, messageReceiver, NULL);

	pthread_join(senderThread, NULL);
	pthread_join(receiverThread, NULL);

	close(socket_desc);

	return 0;
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
		puts("Connected");
		isConnected = 1;
		return socket_desc;	
	}
}

void *messageSender(void *arg){
}

void *messageReceiver(void *arg){
	int receivedMessage;
	struct PACKET receivedPacket;

	while(isConnected) {
		receivedMessage = recv(socket_desc, (void *)&receivedPacket, sizeof(struct PACKET) , 0);
		if(!receivedMessage) {
			puts("Connection with server terminated");
			isConnected = 0;
			pthread_exit(0);
		}	
		if(receivedMessage > 0){
			printf("%s: %s \n", receivedPacket.nickname, receivedPacket.buffer);
		}
		memset(&receivedPacket, 0, sizeof(struct PACKET));
	}
	pthread_exit(0);
}
