
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include <emmintrin.h>

#include "common.h"
#include "spectre.h"

// Buffer to perform timing attack
__attribute__((section(TARGET_SECTION), aligned(ARRSIZE)))
    static const uint8_t array[ARRSIZE] = {'d', 'a', 't', 'a',
                                            [0*4096 + OFFSET] = 0,
                                            [1*4096 + OFFSET] = 1,
                                            [2*4096 + OFFSET] = 2,
                                            [3*4096 + OFFSET] = 3,
                                            [4*4096 + OFFSET] = 4,
                                            [5*4096 + OFFSET] = 5,
                                            [6*4096 + OFFSET] = 6,
                                            [7*4096 + OFFSET] = 7
                                          } ;

// Reference address and the guarding size
int     size     __attribute__((aligned(64))) = REQUEST_SIZE - 1;
uint8_t nums[64] __attribute__((aligned(64)));

// Password to be stolen
uint8_t secret[64];

// Variable representing some internal state of the server
uint8_t state;

// Spectre!
static void victim(int x)
{
    if (x < size && x >= 0)
         state = array[nums[x]*4096 + OFFSET];
}

/*
 * Serves a client (triggers spectre attack inside).
 * Returns wheter the cliend has closed the connection.
 */
static int serve(int client);

int main(int argc, char *argv[])
{
    for (size_t i = 0; i < sizeof(nums); i++)
        nums[i] = i + 1;

    printf("server pid = %d\n", getpid());
    printf("size = %p, nums = %p, secret = %p\n", &size, &nums[0],
        &secret[0]);

    // Some general shit
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }

	// Run server only on core 0
    int err = set_cpu(0);
    if (err < 0)
        return EXIT_FAILURE;

    int port = read_port(argv[1]);
    if (port < 0)
    {
        fprintf(stderr, "Invalid port number: '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Read array in memory...
    for (int i = 0; i < 256; i++)
        state &= array[i*4096 + OFFSET];

    // The hook to make it all look serious
    printf("Password, please: ");
    fgets((char*)secret, sizeof(secret), stdin);

    // Server starts
    int sock = setup_server(port);
    if (sock < 0)
        return EXIT_FAILURE;

    /*
     * 'Main loop' - except for the fact that this server only works
     * with one client and then quits.
     */
    printf("Started server on port %s\n", argv[1]);

    int client = accept(sock, NULL, NULL);
    if (client < 0)
    {
        perror("accept");
        close(sock);
        return EXIT_FAILURE;
    }
    printf("Client connected, serving...\n");

    // Operation shitstorm has begun...
    while (serve(client) == 0)
        ;
    close(client);

    printf("Client done.\n");
    close(sock);
    return EXIT_SUCCESS;
}

int serve(int client)
{
    assert(client > 0);

    // Just receiving 11 numbers to check them... #todo why 11?
    int requests[REQUEST_SIZE];
    int ret = recv(client, requests, sizeof(requests), 0);
    if (ret < 0)
    {
        perror("recv");
        return -1;
    }
    else if (ret == 0)
        return 1;

    // Do some extremely useful work
    for (int i = 0; i < size; i++)
        victim(requests[i]);

    _mm_clflush(&size);
    victim(requests[REQUEST_SIZE - 1]);

    // And return results
    ret = send(client, &state, sizeof(state), 0);
    if (ret < 0)
    {
        perror("send");
        return -1;
    }

    return 0;
}

