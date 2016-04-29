#ifndef _FIGTREE_H_
#define _FIGTREE_H_

#include "interval.h"

typedef void* figtree_value_t;

#define FT_ORDER 5
#define FT_SPLITLIMIT (1 + (FT_ORDER << 1))


typedef struct figtree {
    struct ft_node* root;
} figtree_t;

void ft_init(struct figtree* this);
void ft_write(struct figtree* this, byte_index_t start, byte_index_t end,
              figtree_value_t value);

#endif
