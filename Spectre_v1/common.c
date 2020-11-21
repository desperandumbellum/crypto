
#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "spectre.h"

int read_port(const char *p)
{
    assert(p);

    char *endptr;
    errno = 0;
    int port = strtol(p, &endptr, 10);
    if (port < 1024 || port > 65535 || (*endptr != '\0'))
        return -1;

    return port;
}

static uint8_t dummy;
uint8_t *mapfile(const char *mem, size_t size)
{
    assert(mem);

    int fd = open(mem, O_RDONLY);
    if (!fd)
    {
        perror("open");
        return NULL;
    }

    uint8_t *array = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (array == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return NULL;
    }
    close(fd);

    for (size_t i = 0; i < size; i++)
        dummy &= array[i];

    return array;
}

int setup_client(const char *addr, int port)
{
    assert(addr);
    assert(port < 65536);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(port);
    int err;
    err = inet_pton(AF_INET, addr, &sa.sin_addr.s_addr);
    if (err != 1)
    {
        perror("inet_pton");
        close(sock);
        return -1;
    }

    err = connect(sock, (struct sockaddr*)&sa, sizeof(sa));
    if (err)
    {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

int setup_server(int port)
{
    assert(port < 65536);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    /* Reusing in the following call to avoid creating new variables */
    int err = 1;
    err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &err, sizeof(err));
    if (err < 0)
    {
        perror("setsockopt");
        close(sock);
        return -1;
    }
    
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    err = bind(sock, (struct sockaddr*)&sa, sizeof(sa));
    if (err)
    {
        perror("bind");
        close(sock);
        return -1;
    }

    err = listen(sock, 10);
    if (err)
    {
        perror("listen");
        close(sock);
        return -1;
    }

    return sock;
}
