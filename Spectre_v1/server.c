
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include <emmintrin.h>

#include "common.h"
#include "spectre.h"

// Buffer to attack
__attribute__((section(TARGET_SECTION), aligned(ARRSIZE)))
    static const uint8_t array[ARRSIZE] = {'h', 'u', 'i'} ;

// Some data
int     size     __attribute__((aligned(64))) = REQUEST_SIZE - 1;
uint8_t nums[64] __attribute__((aligned(64)));

// Password to be stolen
uint8_t secret[64];
uint8_t state;

// Spectre
static void fuck_up(int x)
{
    if (x < size)
         state = array[nums[x]*4096 + OFFSET];
}

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
    {
        // munmap(array, ARRSIZE);
        return EXIT_FAILURE;
    }

    // Main loop
    printf("Started server on port %s, try to fuck me\n", argv[1]);
    while (1)
    {
        int client = accept(sock, NULL, NULL);
        if (client < 0)
        {
            perror("accept");
            // munmap(array, ARRSIZE);
            return EXIT_FAILURE;
        }
        printf("Somebody connected...\n");

        // Operation shitstorm has begun...
        while (1)
            serve(client);
        close(client);
    }

    // munmap(array, ARRSIZE);
    return EXIT_SUCCESS;
}

/*
 * Does not actually check sends and receives properly
 */
int serve(int client)
{
    assert(client > 0);

    // Just receiving 11 numbers to check them...
    int requests[REQUEST_SIZE];
    int ret = recv(client, requests, sizeof(requests), 0);
    if (ret < 0)
    {
        perror("recv");
        return -1;
    }

    // Do some extremely useful work
    for (int i = 0; i < size; i++)
        fuck_up(requests[i]);

    _mm_clflush(&size);
    fuck_up(requests[REQUEST_SIZE - 1]);

    // And return results
    ret = send(client, &state, sizeof(state), 0);
    if (ret < 0)
    {
        perror("send");
        return -1;
    }

    return 0;
}

