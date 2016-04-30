#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

void my_assert(bool x, char* desc) {
    if (!x) {
        fprintf(stderr, "Assertion failed: %s\n", desc);
        exit(1);
    }
}

void* mem_alloc(size_t s) {
    return calloc(s, 1);
}

void mem_free(void* ptr) {
    free(ptr);
}

void subtree_set(struct subtree_ptr* sptr, struct ft_node* st) {
    sptr->st = st;
}

struct ft_node* subtree_get(struct subtree_ptr* sptr) {
    return sptr->st;
}
