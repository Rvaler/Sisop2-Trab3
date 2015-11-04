#define ROOMNAMELENGHT 64
#define NICKLENGHT 32
#define MESSAGE_SIZE 256
#define ERROR -1
#define MAXCLIENTS 15
#define IP "127.0.0.1"
#define PORT 8080
// Tipos de pacote
#define CHANGENICK 1
#define MESSAGE 2
#define CREATE 3
#define LIST 4
#define JOIN 5
#define LEAVE 5
#define QUIT 6
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
