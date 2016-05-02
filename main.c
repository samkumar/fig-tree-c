#include <stdio.h>
#include <stdlib.h>

#include "figtree.h"
#include "interval.h"

#define BYTE_INDEX_BITS 8
#define MAX_FILE_SIZE (1 << BYTE_INDEX_BITS)
#define BYTE_INDEX_MASK (MAX_FILE_SIZE - 1)

#define PRINT_FREQ_BITS 6
#define PRINT_FREQ_MASK ((1 << PRINT_FREQ_BITS) - 1)

#define NUM_ITERATIONS (0x400000)

#define MAGIC 0xb96904ab6ff9f2f5

void test_figtree(unsigned int seed) {
    figtree_t ft;
    int i;
    byte_index_t j;
    figtree_value_t file[MAX_FILE_SIZE];
    figtree_value_t* res;

    for (j = 0; j < MAX_FILE_SIZE; j++) {
        file[j] = MAGIC;
    }
    ft_init(&ft);

    i = 0;
    while (i < NUM_ITERATIONS) {
        if (((++i) & PRINT_FREQ_MASK) == 0) {
            printf("Iteration 0x%x (%d)\n", i, i);
        }
        byte_index_t start = (byte_index_t) (rand_r(&seed) & BYTE_INDEX_MASK);
        byte_index_t end = (byte_index_t) (rand_r(&seed) & BYTE_INDEX_MASK);
        figtree_value_t value = (figtree_value_t) rand_r(&seed);
        if (start > end) {
            byte_index_t temp = start;
            start = end;
            end = temp;
        }
        ft_write(&ft, start, end, value);
        for (j = start; j <= end; j++) {
            file[j] = value;
        }
        
        for (j = 0; j < MAX_FILE_SIZE; j++) {
            res = ft_lookup(&ft, j);
            if (res == NULL) {
                value = MAGIC;
            } else {
                value = *res;
            }
            if (value != file[j]) {
                fprintf(stderr,
                        "[ERROR] Byte %lu is not 0x%x (instead, it is 0x%x)\n",
                        (long unsigned int) j, (unsigned int) file[j],
                        (unsigned int) value);
                goto done;
            }
        }
        for (start = 0; start < MAX_FILE_SIZE; start++) {
            for (end = start; end < MAX_FILE_SIZE; end++) {
                figiter_t* figiter;
                fig_t fig;

                figiter = ft_read(&ft, start, end);
                j = start;
                while (fti_next(figiter, &fig)) {
                    /* Verify that the fig is in bounds. */
                    if (fig.irange.left < start || fig.irange.right > end
                        || fig.irange.left > fig.irange.right) {
                        fprintf(stderr, "[ERROR] Invalid fig [%lu, %lu]\n",
                                (long unsigned int) fig.irange.left,
                                (long unsigned int) fig.irange.right);
                    }
                    /* Verify that there's nothing in the file before the
                     * range.
                     */
                    for (; j < fig.irange.left; j++) {
                        if (file[j] != (figtree_value_t) MAGIC) {
                            fprintf(stderr,
                                    "[ERROR] Byte %lu is not 0x%x (no range)\n",
                                    (long unsigned int) j,
                                    (unsigned int) file[j]);
                        }
                    }
                    /* Verify that everything in the range is correct. */
                    for (; j <= fig.irange.right; j++) {
                        if (file[j] != fig.value) {
                            fprintf(stderr,
                                    "[ERROR] Byte %lu is not 0x%x (got 0x%x)\n",
                                    (long unsigned int) j,
                                    (unsigned int) file[j],
                                    (unsigned int) value);
                        }
                    }
                }
                /* Make sure that there's nothing past the last range. */
                for (; j <= end; j++) {
                    if (file[j] != (figtree_value_t) MAGIC) {
                        fprintf(stderr,
                                "File at byte %lu is not 0x%x (no range)\n",
                                (long unsigned int) j, (unsigned int) file[j]);
                    }
                }
                fti_free(figiter);
            }
        }
    }

    done:
    printf("Deallocating the Fig Tree...\n");
    ft_dealloc(&ft);

    printf("Done.\n");
}

int main(int argc, char** argv) {
    unsigned int seed;
    struct interval i;
    i_init(&i, 1, 7);
    ASSERT(i_contains_val(&i, 7), "Sanity check failed");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <seed>\n", argv[0]);
        return 1;
    }
    
    seed = (unsigned int) strtol(argv[1], NULL, 10);
    printf("Using seed = %lu\n", (long unsigned int) seed);
    test_figtree(seed);
}
