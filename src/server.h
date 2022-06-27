#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

struct server_t
{
    int fd;
    int port;
    sockaddr_in address;
};

typedef struct server_t server_t;

server_t server_create(int port);
int server_listen(server_t *server);
int server_accept(server_t *server);
int server_send(int client, char *data, size_t size);
char *server_recieve(int client);

#endif