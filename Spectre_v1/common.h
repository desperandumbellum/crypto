
#ifndef   COMMON_H_INCLUDED
#define   COMMON_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>

int read_port(const char *p);
uint8_t *mapfile(const char *mem, size_t size);
int setup_client(const char *addr, int port);
int setup_server(int port);

#endif // COMMON_H_INCLUDED

