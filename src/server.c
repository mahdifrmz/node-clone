#include <stdlib.h>
#include <server.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

server_t server_create(int port)
{
    server_t server;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == 0)
    {
        fd = -1;
    }
    int socket_opt_value = 1;
    int rsl = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt_value, sizeof(int));
    if (rsl == -1)
    {
        fd = -1;
    }
    struct sockaddr_in address;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    rsl = bind(fd, (sockaddr *)&address, sizeof(sockaddr_in));
    if (rsl == -1)
    {
        fd = -1;
    }
    server.fd = fd;
    server.address = address;
    server.port = port;
    return server;
}

int server_listen(server_t *server)
{
    return listen(server->fd, 1024);
}

int server_accept(server_t *server)
{
    socklen_t len = sizeof(sockaddr_in);
    return accept(server->fd, (sockaddr *)&server->address, &len);
}

int server_send(int client, char *data, size_t size)
{
    ssize_t rsl = write(client, data, size);
    if (rsl > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

char *server_recieve(int client)
{
    char buffer[1024];
    ssize_t rsl = read(client, buffer, 1024);
    buffer[rsl] = '\0';
    if (rsl >= 0)
    {
        return strdup(buffer);
    }
    else
    {
        return NULL;
    }
}