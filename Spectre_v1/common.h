
#ifndef   COMMON_H_INCLUDED
#define   COMMON_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>

int read_port(const char *p);
/*
 * file - make of the file to be mapped
 */
uint8_t *mapfile(const char *file, off_t offset, size_t length);
/*
 * mem - array with file contents
 */
ssize_t secoffset(void *mem, const char *secname, size_t *secsize);
uint8_t *mapsection(const char *file, const char *secname, size_t secsize);
int setup_client(const char *addr, int port);
int setup_server(int port);

#endif // COMMON_H_INCLUDED

