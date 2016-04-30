#include <stdio.h>
#include <stdlib.h>

#include "figtree.h"
#include "interval.h"

#define BYTE_INDEX_BITS 16
#define MAX_FILE_SIZE (1 << BYTE_INDEX_BITS)
#define BYTE_INDEX_MASK (MAX_FILE_SIZE - 1)

#define PRINT_FREQ_BITS 20
#define PRINT_FREQ_MASK ((1 << PRINT_FREQ_BITS) - 1)

#define NUM_ITERATIONS (0x800000)

void test_figtree(unsigned int seed) {
    struct figtree ft;
    int i;
    ft_init(&ft);

    i = 0;
    while (i < NUM_ITERATIONS) {
        if (((++i) & PRINT_FREQ_MASK) == 0) {
            printf("Iteration 0x%x (%d)\n", i, i);
        }
        byte_index_t start = (byte_index_t) (rand_r(&seed) & BYTE_INDEX_MASK);
        byte_index_t end = (byte_index_t) (rand_r(&seed) & BYTE_INDEX_MASK);
        int value = rand_r(&seed);
        if (start < end) {
            byte_index_t temp = start;
            start = end;
            end = temp;
        }

        ft_write(&ft, start, end, value);
    }

    printf("Deallocating the Fig Tree...\n");
    ft_dealloc(&ft);

    printf("Done.\n");
}

int main(int argc, char** argv) {
    unsigned int seed;
    struct interval i;
    i_init(&i, 1, 7);
    ASSERT(!i_contains_val(&i, 7), "Sanity check failed");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <seed>\n", argv[0]);
        return 1;
    }
    
    seed = (unsigned int) strtol(argv[1], NULL, 10);
    printf("Using seed = %lu\n", (long unsigned int) seed);
    test_figtree(seed);
}
