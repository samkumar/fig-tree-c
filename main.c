#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "figtree.h"
#include "figtreenode.h"
#include "interval.h"

#define BYTE_INDEX_BITS 11
#define MAX_FILE_SIZE (1 << BYTE_INDEX_BITS)
#define BYTE_INDEX_MASK (MAX_FILE_SIZE - 1)

#define PRINT_FREQ_BITS 4
#define PRINT_FREQ_MASK ((1 << PRINT_FREQ_BITS) - 1)

#define MAX_WRITE_BITS 5
#define MAX_WRITE_MASK ((1 << MAX_WRITE_BITS) - 1)

#define NUM_ITERATIONS (0x100)

#define MAGIC 0xb96904ab6ff9f2f5

unsigned int test_figtree(unsigned int seed, int threadid) {
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
            printf("Thread %d: iteration 0x%x (%d) [height = %d]\n", threadid,
                   i, i, ft.root->HEIGHT);
        }
        byte_index_t start = (byte_index_t) (rand_r(&seed) & BYTE_INDEX_MASK);
        byte_index_t end = start +
            (byte_index_t) (rand_r(&seed) & MAX_WRITE_MASK);
        figtree_value_t value = (figtree_value_t) rand_r(&seed);
        if (end >= MAX_FILE_SIZE) {
            end = MAX_FILE_SIZE - 1;
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

    return seed;
}

struct test_args {
    unsigned int seed;
    int threadid;
};

void* test_start(void* _args) {
    struct test_args* args = _args;
    unsigned int seed = args->seed;
    int threadid = args->threadid;
    long unsigned int i = 1;
    while (true) {
        printf("THREAD %d: ROUND %lu [seed = %u]\n", threadid, i, seed);
        seed = test_figtree(seed, threadid);
        i++;
    }
    return NULL;
}

int main(int argc, char** argv) {
    int numthreads = argc - 1;
    struct interval i;
    int c;
    struct test_args targs[numthreads];
    pthread_t realids[numthreads];
    pthread_attr_t attr;
    
    i_init(&i, 1, 7);
    ASSERT(i_contains_val(&i, 7), "Sanity check failed");

    if (argc < 2) {
        fprintf(stderr,
                "Usage: %s <seed for thread 1> <seed for thread 2> ...\n",
                argv[0]);
        return 1;
    }

    for (c = 1; c <= numthreads; c++) {
        targs[c - 1].seed = (unsigned int) strtol(argv[c], NULL, 10);
        targs[c - 1].threadid = c;
        printf("Using seed = %u for thread %d\n", targs[c - 1].seed, c);
    }

    ASSERT(pthread_attr_init(&attr) == 0, "Could not initialize pthread_attr");
    ASSERT(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0,
           "Could not set detach state of pthread_attr");
    
    for (c = 0; c < numthreads; c++) {
        errno = pthread_create(&realids[c], &attr, test_start, &targs[c]);
        if (errno) {
            perror("Could not create thread");
        }
    }
    for (c = 0; c < numthreads; c++) {
        printf("Thread %d has id %lu\n", c + 1, realids[c]);
    }

    ASSERT(pthread_attr_destroy(&attr) == 0, "Could not destroy pthread_attr");

    /* The test_args are on this thread's stack... so we need to keep this
     * thread running in order to avoid race conditions.
     */
    for (c = 0; c < numthreads; c++) {
        pthread_join(realids[c], NULL);
    }
}
