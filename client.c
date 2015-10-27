
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define MESSAGE_SIZE 256

int socket_desc;
int port;
struct hostent *server;
pthread_t receiverThread, senderThread;
//char *inAddress = "201.21.215.114";


int connectToServer(int argc, char *argv[]);
void *messageReceiver(void *arg);
void *messageSender(void *arg);

int main(int argc , char *argv[])
{
	if(verifyInput(argc, argv) < 0)
		return -1;

	if(connectToServer(argc, argv) == -1){
		puts("Error in connection");
		return -1;
	}


	pthread_create(&senderThread, NULL, messageSender, NULL);
	pthread_create(&receiverThread, NULL, messageReceiver, NULL);

	pthread_join(senderThread, NULL);
	pthread_join(receiverThread, NULL);

	close(socket_desc);
	/*
	int socket_desc;
	struct sockaddr_in server;
	char *message, server_reply[2000];
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc < 0)
	{
		printf("Could not create socket");
	}
		
	server.sin_addr.s_addr = inet_addr(google);
	server.sin_family = AF_INET;
	server.sin_port = htons( 80 );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("connect error");
		return 1;
	}
	puts("Connected");


	//Send some data
	message = "GET / HTTP/1.1\r\n\r\n";
	if( send(socket_desc , message , strlen(message) , 0) < 0)
	{
		puts("Send failed");
		return 1;
	}
	puts("Data Send\n");
	
	//Receive a reply from the server
	if( recv(socket_desc, server_reply , 2000 , 0) < 0)
	{
		puts("recv failed");
	}
	puts("Reply received\n");
	puts(server_reply);

	close(socket_desc);
	puts("Closed socket");
	*/
	return 0;
}

int verifyInput(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Error - Missing arguments!\nUse:./client hostname portNumber\n");
		return -1;
	}
}

int connectToServer(int argc, char *argv[]) {

	struct sockaddr_in server_addr;
	port = atoi(argv[2]);
	server = gethostbyname(argv[1]);

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc < 0)
	{
		printf("Could not create socket");
		return -1;
	}

	server_addr.sin_addr = *((struct in_addr *)server->h_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons( port );

	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)
	{
		puts("connect error");
		return -1;
	}
	puts("Connected");
	return 0;
}

void *messageSender(void *arg){
}

void *messageReceiver(void *arg){
	char buffer[MESSAGE_SIZE];
	//Receive a reply from the server
	if( recv(socket_desc, buffer , MESSAGE_SIZE , 0) < 0)
	{
		puts("recv failed");
	}
	puts(buffer);

    pthread_exit(0);
}
