
#ifndef   COMMON_H_INCLUDED
#define   COMMON_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>

/*
 * Reads int from a string pointed by 'p'. The port number
 *  must lie between 1024 and 65535.
 * Returns port number or (-1) in case of failure.
 */
int read_port(const char *p);
/*
 * Maps file to the process memory.
 *  'file' - make of the file to be mapped, 'offset' and 'length' correspond
 *  to the arguments to mmap() call.
 * Returns address of the mapped region or NULL in case of failure.
 */
uint8_t *mapfile(const char *file, off_t offset, size_t length);
/*
 * Maps section with name 'secname' of size 'secsize' to the process memory.
 *  Stores section size in 'secsize' if it is not NULL. Expects the file to
 *  be already placed in memory with 'mem' pointer set to its beginning.
 * Returns offset of the requested section from the beginning of the file or
 * (-1) in case of failure.
 */
ssize_t secoffset(void *mem, const char *secname, size_t *secsize);
/*
 * Maps the section with name 'secname' of size 'secsize' from the ELF file
 *  'file'.
 * Returns the pointer to the mapped region or NULL in case of failure.
 */
uint8_t *mapsection(const char *file, const char *secname, size_t secsize);

/*
 * Starts a TCP client: creates a TCP socket connected to the 'addr':'port'
 *  network address.
 * Retuns the socket or (-1) in case of failure.
 */
int setup_client(const char *addr, int port);
/*
 * Starts a TCP server: creates a TCP socket listening on localhost:'port'
 *  network address.
 * Retuns the socket or (-1) in case of failure.
 */
int setup_server(int port);

/*
 * Sets cpu affinity of the calling process.
 * Returns whether the operation succedded.
 */
int set_cpu(int cpu);
/*
 * Estimates the threshold to distinguish cache hit and cache miss.
 */
uint64_t calculate_threshold(int runs);
/*
 * Returns amount of cycles it took to access the 'target' address.
 */
uint64_t measure_access_time(uint8_t *target);

#endif // COMMON_H_INCLUDED

