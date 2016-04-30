#ifndef _FIGTREE_H_
#define _FIGTREE_H_

#include "interval.h"
#include "utils.h"

typedef long int figtree_value_t;

#define FT_ORDER 5
#define FT_SPLITLIMIT (1 + (FT_ORDER << 1))

void ftn_free(struct ft_node* this);

typedef struct figtree {
    struct ft_node* root;
} figtree_t;

/* Initializes a Fig Tree in the specified space. */
void ft_init(struct figtree* this);

/* Sets the bytes in the range [START, END] to correspond to VALUE. */
void ft_write(struct figtree* this, byte_index_t start, byte_index_t end,
              figtree_value_t value);

/* Deallocates the resources for the Fig Tree in the specified space. */
void ft_dealloc(struct figtree* this);

#endif
