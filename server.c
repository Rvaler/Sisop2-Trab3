#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>	
#include <unistd.h>
#include <pthread.h>


#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>


struct PACKET {
	int roomID;
	char nickname[32];
	char buffer[256];
};


void *connection_handler(void *);

int main(int argc,char *argv[]){

	int socket_desc, new_socket , c, *new_sock;
	struct sockaddr_in server, client;
	char *message;
     
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
   		printf("Could not create socket");
	}
     
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );
     
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
   		puts("bind failed");
	}
	puts("bind done");

	//Listen
	listen(socket_desc , 3); // 3 = max_connections
	//Accept and incoming connection
    puts("Waiting for incoming connections...");

    c = sizeof(struct sockaddr_in);
    while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        //Reply to the client
        message = "Hello Client , I have received your connection. But I have to go now, bye\n";
        write(new_socket , message , strlen(message));

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;
         
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }

    if (new_socket < 0) {
        perror("accept failed");
        return 1;
    }

	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[2000];
     
    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));
     
    message = "Now type something and i shall repeat what you type \n";
    write(sock , message , strlen(message));
     
    //Receive a message from client
    while( (read_size = recv(sock , (void *)&client_message , 2000 , 0)) > 0 )
    {
        //Send the message back to client

        puts(client_message);
		struct PACKET packet;

		char *msg = client_message;
		memset(&packet, 0, sizeof(struct PACKET));
		strcpy(packet.nickname, "Rafael");
		strcpy(packet.buffer, msg);
        puts("entrou aqui");
        int a = send(sock, (void *)&packet, sizeof(struct PACKET), 0);
		printf("%i\n", a);
        //send(socket_desc, client_message, 200, 0);
        //write(sock , client_message , strlen(client_message));
    }
     
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    //Free the socket pointer
    free(socket_desc);
     
    return 0;
}