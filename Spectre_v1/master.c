
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>

#include "common.h"
#include "spectre.h"

int main(int argc, char *argv[])
{
    // Some general shit
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = read_port(argv[1]);
    if (port < 0)
    {
        fprintf(stderr, "Invalid port number: '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    int server = setup_client("127.0.0.1", port);
    if (server < 0)
    {
        fprintf(stderr, "Connection to server failed\n");
        return EXIT_FAILURE;
    }
    int slave = setup_client("127.0.0.1", CONTROL_PORT);
    if (slave < 0)
    {
        fprintf(stderr, "Connection to slave failed\n");
        close(server);
        return EXIT_FAILURE;
    }

    // Main loop
    printf("Connections set up, let's go!\n");
    while (1)
    {
        uint8_t requests[REQUEST_SIZE] = {};
        int offset;

        printf("Enter offset: ");
        int ret = scanf("%d", &offset);
        if (ret == EOF)
        {
            printf("\n");
            break;
        }
        requests[REQUEST_SIZE - 1] = offset;

        send(server, requests, sizeof(requests), 0);
        send(slave,  requests, sizeof(requests), 0);
        // Just prevent internal buffers fillings
        recv(server, requests, sizeof(requests), 0);
    }

    close(server);
    close(slave);
    return EXIT_SUCCESS;
}

