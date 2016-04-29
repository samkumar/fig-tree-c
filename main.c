#include <stdio.h>
#include "figtree.h"
#include "interval.h"

int main(void) {
    struct interval i;
    struct figtree ft;
    i_init(&i, 1, 7);
    printf("%d\n", i_contains_val(&i, 7));

    ft_init(&ft);
    ft_write(&ft, &i, &i);
}
