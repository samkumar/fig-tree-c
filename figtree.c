#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "figtree.h"
#include "interval.h"
#include "utils.h"

/* Fig Tree Entry */

struct ft_ent {
    struct interval irange;
    figtree_value_t value;
};

bool fte_overlaps(struct ft_ent* this, struct ft_ent* other) {
    return i_overlaps(&this->irange, &other->irange);
}


/* Fig Tree Node */

struct ft_node {
    int entries_len;
    struct ft_ent entries[FT_SPLITLIMIT];
    int subtrees_len;
    struct subtree_ptr subtrees[FT_SPLITLIMIT + 1];
    int HEIGHT;
};

void _ftn_entries_add(struct ft_node* this, int index, struct ft_ent* new) {
    ASSERT(index >= 0 && index <= this->entries_len &&
           this->entries_len < FT_SPLITLIMIT, "Bad index in _ftn_entries_add");
    memmove(&this->entries[index + 1], &this->entries[index],
            (this->entries_len - index) * sizeof(struct ft_ent));
    memcpy(&this->entries[index], new, sizeof(struct ft_ent));
    this->entries_len++;
}   

void _ftn_subtrees_add(struct ft_node* this, int index, struct ft_node* new) {
    ASSERT(index >= 0 && index <= this->subtrees_len &&
           this->subtrees_len <= FT_SPLITLIMIT,
           "Bad index in _ftn_subtrees_add");
    memmove(&this->subtrees[index + 1], &this->subtrees[index],
            (this->subtrees_len - index) * sizeof(struct ft_node*));
    subtree_set(&this->subtrees[index], new);
    this->subtrees_len++;
}

void ftn_clear(struct ft_node* this, bool make_height);

struct ft_node* ftn_new(int height, bool make_height) {
    struct ft_node* this;
    ASSERT(height >= 0, "Negative height in ftn_new");
    this = mem_alloc(sizeof(struct ft_node));
    this->HEIGHT = height;

    ftn_clear(this, make_height);
    
    return this;
}

void ftn_clear(struct ft_node* this, bool make_height) {
    struct ft_node* firstchild;
    int i;
    for (i = 0; i < this->subtrees_len; i++) {
        subtree_free(&this->subtrees[i]);
    }
    
    if (this->HEIGHT == 0 || !make_height) {
        firstchild = NULL;
    } else {
        firstchild = ftn_new(this->HEIGHT - 1, true);
    }
    this->entries_len = 0;
    subtree_set(&this->subtrees[0], firstchild);
    this->subtrees_len = 1;
}

struct ft_node* ftn_insert(struct ft_node* this, struct ft_ent* newent,
                           int index, struct ft_node* leftChild,
                           struct ft_node* rightChild) {
    ASSERT(this->entries_len + 1 == this->subtrees_len, "entries-subtree invariant violated in ftn_insert");
    ASSERT(index >= 0 && index <= this->entries_len &&
           (index == 0 || !fte_overlaps(newent, &this->entries[index - 1])) &&
           (index == this->entries_len ||
            !fte_overlaps(newent, &this->entries[index])), "bad ftn_insert");
    _ftn_entries_add(this, index, newent);
    subtree_set(&this->subtrees[index], leftChild);
    _ftn_subtrees_add(this, index + 1, rightChild);

    if (this->entries_len == FT_SPLITLIMIT) {
        // Split the node and push the middle entry to parent
        struct ft_node* left = ftn_new(this->HEIGHT, false);
        struct ft_node* right = ftn_new(this->HEIGHT, false);
        int i;

        left->entries_len = FT_ORDER;
        left->subtrees_len = FT_ORDER + 1;
        right->entries_len = FT_ORDER;
        right->subtrees_len = FT_ORDER + 1;
        left->subtrees[0] = this->subtrees[0];
        right->subtrees[0] = this->subtrees[FT_ORDER + 1];
        for (i = 0; i < FT_ORDER; i++) {
            left->entries[i] = this->entries[i];
            left->subtrees[i + 1] = this->subtrees[i + 1];
            right->entries[i] = this->entries[FT_ORDER + i + 1];
            right->subtrees[i + 1] = this->subtrees[FT_ORDER + i + 2];
        }

        /* This node no longe exists... so we might as well reuse its memory for
         * the node that got pushed up the tree.
         */
        this->entries[0] = this->entries[FT_ORDER];
        this->entries_len = 1;
        subtree_set(&this->subtrees[0], left);
        subtree_set(&this->subtrees[1], right);
        this->subtrees_len = 2;
        this->HEIGHT++;

        return this;
    }

    return NULL;
}

void ftn_replaceEntries(struct ft_node* this, int start, int end,
                        struct interval* newent_interval,
                        figtree_value_t newentry_value) {
    ASSERT(this->entries_len + 1 == this->subtrees_len, "entry-subtree invariant violated in ftn_replaceEntries");
    ASSERT(start >= 0 && start < this->entries_len
           && end >= 0 && end <= this->entries_len, "bad ftn_replaceEntries");
    
    memcpy(&this->entries[start].irange, newent_interval,
           sizeof(struct interval));
    this->entries[start].value = newentry_value;
    
    memmove(&this->entries[start + 1], &this->entries[end],
            (this->entries_len - end) * sizeof(struct ft_ent));
    memmove(&this->subtrees[start + 1], &this->subtrees[end],
            (this->subtrees_len - end) * sizeof(struct ft_node*));
}

void ftn_pruneTo(struct ft_node* this, struct interval* valid) {
    struct ft_node* true_subtree;
    struct subtree_ptr* subtree;
    struct interval* entryint;
    
    bool entrydel[this->entries_len];
    bool subtreedel[this->subtrees_len];
    int i = 0;
    int j;
    
    if (!valid->nonempty) {
        ftn_clear(this, true);
        return;
    }

    if (this->entries_len == 0) {
        return;
    }

    memset(entrydel, 0x00, this->entries_len);
    memset(subtreedel, 0x00, this->subtrees_len);

    entryint = &this->entries[i].irange;
    subtree = &this->subtrees[i];

    while (i_leftOf_int(entryint, valid)) {
        entrydel[i] = true;
        subtreedel[i] = true;

        ASSERT(i < this->entries_len, "iterated past end of entries (#1)");
        if (++i == this->entries_len) {
            goto performdeletes;
        }
        entryint = &this->entries[i].irange;
        subtree = &this->subtrees[i];
    }

    if (i_leftOverlaps(valid, entryint)) {
        if ((true_subtree = subtree_get(subtree)) != NULL) {
            ftn_clear(true_subtree, true);
        }

        // In case the valid boundary is in the middle of this interval
        i_restrict_int(entryint, valid, false);

        ASSERT(i < this->entries_len, "iterated past end of entries (#2)");
        if (++i == this->entries_len) {
            goto performdeletes;
        }
        entryint = &this->entries[i].irange;
        subtree = &this->subtrees[i];
    }

    while (i_contains_int(valid, entryint)) {
        ASSERT(i < this->entries_len, "iterated past end of entries (#3)");
        if (++i == this->entries_len) {
            goto performdeletes;
        }
        entryint = &this->entries[i].irange;
        subtree = &this->subtrees[i];
    }

    /* At this point, entryint is either overlapping with the right edge of
     * VALID, or it is beyond VALID. The left subtree, in either case, cannot
     * be dropped, since part of it must be in the valid interval. So, just
     * skip over it. From now on, we need to index subtrees with i + 1 (so
     * that SUBTREE is the right subtree of ENTRY).
     */
    subtree = &this->subtrees[i + 1];

    if (i_rightOverlaps(valid, entryint)) {
        if ((true_subtree = subtree_get(subtree)) != NULL) {
            ftn_clear(true_subtree, true);
        }

        // In case the valid boundary is in the middle of this interval
        i_restrict_int(entryint, valid, false);

        ASSERT(i < this->entries_len, "iterated past end of entries (#4)");
        if (++i == this->entries_len) {
            goto performdeletes;
        }
        entryint = &this->entries[i].irange;
        subtree = &this->subtrees[i + 1];
    }

    while (i_rightOf_int(entryint, valid)) {
        entrydel[i] = true;
        subtreedel[i + 1] = true;

        ASSERT(i < this->entries_len, "iterated past end of entries (#5)");
        if (++i == this->entries_len) {
            goto performdeletes;
        }
        entryint = &this->entries[i].irange;
        // don't assign to subtree, since henceforth we only need to remove
    }   

    performdeletes:
    for (i = 0, j = 0; i < this->entries_len; i++) {
        if (!entrydel[i]) {
            this->entries[j] = this->entries[i];
            j++;
        }
    }
    this->entries_len = j;
    for (i = 0, j = 0; i < this->subtrees_len; i++) {
        if (subtreedel[i]) {
            subtree_free(&this->subtrees[i]);
        } else {
            this->subtrees[j] = this->subtrees[i];
            j++;
        }
    }
    this->subtrees_len = j;

    ASSERT(this->entries_len + 1 == this->subtrees_len,
           "entries-subtree invariant violated after pruning");
}

void ftn_free(struct ft_node* this) {
    /* First, free all subtrees. */
    int i;
    for (i = 0; i < this->subtrees_len; i++) {
        subtree_free(&this->subtrees[i]);
    }

    /* Then, free this node. */
    mem_free(this);
}


/* Fig Tree */

void ft_init(struct figtree* this) {
    this->root = ftn_new(0, true);
}

struct insertargs {
    struct interval range;
    figtree_value_t value;
    struct ft_node** path;
    int path_len;
    int* pathIndices;
    int pathIndices_len;
    struct ft_node* at;
    struct interval valid;
};

struct insertcont {
    bool hasleftc;
    struct insertargs leftc;
    bool hasrightc;
    struct insertargs rightc;
};

void _ft_insert(struct figtree* this, struct insertargs* args,
                bool rightcontinuation, struct insertcont* ic) {
    struct interval* range = &args->range;
    figtree_value_t value = args->value;
    struct ft_node** path = args->path;
    int* pathIndices = args->pathIndices;
    struct ft_node* currnode = args->at;
    struct interval* valid = &args->valid;

    int finalsharedindex = args->pathIndices_len - 1;

    /* Record the residual groups [star1, range->left - 1] and
       [range->right + 1, star2]. */

    int numentries, i;

    memset(ic, 0x00, sizeof(struct insertcont));

    outerloop:
    while (currnode != NULL) {
        struct ft_ent* current;
        struct interval* previval;
        struct interval* currival;
        ftn_pruneTo(currnode, valid);
        numentries = currnode->entries_len;
        current = NULL;
        currival = NULL;

        for (i = 0; i < numentries; i++) {
            previval = currival;
            current = &currnode->entries[i];
            currival = &current->irange;
            if (i_overlaps(currival, range)) {
                int j;
                struct ft_ent* previous;
                
                path[args->path_len++] = currnode;
                if (currival->left < range->left) {
                    // Create a continuation for the left subinterval
                    ic->hasleftc = true;
                    i_init(&ic->leftc.range, currival->left, range->left - 1);
                    ic->leftc.value = current->value;
                    ic->leftc.path = path;
                    ic->leftc.path_len = args->path_len;
                    ic->leftc.pathIndices = pathIndices;
                    // A new entry is to be added to pathIndices later
                    ic->leftc.pathIndices_len = args->pathIndices_len + 1;
                    ic->leftc.at = subtree_get(&currnode->subtrees[i]);
                    memcpy(&ic->leftc.valid, valid, sizeof(struct interval));
                    i_restrict_range(&ic->leftc.valid, i == 0 ? BYTE_INDEX_MIN :
                                     previval->right + 1,
                                     currival->left - 1, true);
                }
                
                /* The entry in this node immediately after current will either
                 * be disjoint from RANGE, or will left-overlap it. It can't
                 * right-overlap it (unless it also left-overlaps it).
                 */
                previous = current;
                for (j = i + 1; j < numentries &&
                         i_leftOverlaps(&(current =
                                          &currnode->entries[j])->irange,
                                        range); j++) {
                    previous = current;
                }

                /* Now, either CURRENT is the first entry in the node that is
                 * disjoint from RANGE, or, if there is no such entry, then
                 * j == numentries. In either case, PREVIOUS is the last entry
                 * in the node that overlaps with RANGE.
                 */
                ftn_replaceEntries(currnode, i, j, range, value);
                if (previous->irange.right > range->right) {
                    // Create a continuation for the right subinterval
                    ic->hasrightc = true;
                    i_init(&ic->rightc.range, range->right + 1,
                           previous->irange.right);
                    ic->rightc.value = previous->value;
                    ic->rightc.path = path;
                    ic->rightc.path_len = args->path_len;
                    ic->rightc.pathIndices = pathIndices;
                    // A new entry is to be added to pathIndices later
                    ic->rightc.pathIndices_len = args->pathIndices_len + 1;
                    ic->rightc.at = subtree_get(&currnode->subtrees[i + 1]);
                    memcpy(&ic->rightc.valid, valid, sizeof(struct interval));
                    i_restrict_range(&ic->rightc.valid,
                                     previous->irange.right + 1,
                                     j == numentries ? BYTE_INDEX_MAX :
                                     current->irange.left - 1, true);
                    /* If there's a right continuation, then we set the
                     * path index to that of the right continuation. */
                    /* If there's also a left continuation, then we adjust the
                     * index for the left continuation. */
                } else {
                    /* There's no right continuation that will adjust the final
                     * shared path index for the left continuation. So, we need
                     * to directly insert the index for the left continuation
                     * here, in case there is a left continuation.
                     */
                    pathIndices[args->pathIndices_len++] = i;
                }
		goto treeinsertion;
            } else if (i_rightOf_int(currival, range)) {
                path[args->path_len++] = currnode;
                pathIndices[args->pathIndices_len++] = i;
                currnode = subtree_get(&currnode->subtrees[i]);
                /* What if previval and currival are adjacent intervals? Then
                 * the entire subtree can be pruned. This is represented by the
                 * special empty interval.
                 */
                i_restrict_range(valid, previval == NULL ? BYTE_INDEX_MIN :
                                 previval->right + 1, currival->left - 1, true);
                goto outerloop;
            }
        }
        path[args->path_len++] = currnode;
        pathIndices[args->pathIndices_len++] = numentries;
        currnode = subtree_get(&currnode->subtrees[numentries]);
        i_restrict_range(valid, currival == NULL ? BYTE_INDEX_MIN :
                         currival->right + 1, BYTE_INDEX_MAX, true);
    }

    treeinsertion:
    if (currnode == NULL) {
        // In this case, we actually need to do an insertion
        struct ft_ent toinsert;
        struct ft_node* rv = NULL;
        struct ft_node* topushnode = NULL;
        struct ft_ent* topushent = &toinsert;
        struct ft_node* left = NULL;
        struct ft_node* right = NULL;
        struct ft_node* insertinto;
        int insertindex;
        int pathindex;

        memcpy(&toinsert.irange, range, sizeof(struct interval));
        toinsert.value = value;

        for (pathindex =
                 args->pathIndices_len - 1; pathindex >= 0; pathindex--) {
            insertinto = path[pathindex];
            insertindex = pathIndices[pathindex];
            rv = ftn_insert(insertinto, topushent, insertindex, left, right);
            mem_free(topushnode);
            topushnode = rv;
            
            if (rightcontinuation) {
                /*
                 * All indices in the pathIndices and path lists at or before
                 * FINALSHAREDINDEX are shared with the path in a left
                 * continuation that has not yet been executed. If any nodes get
                 * split along that path, we need to update the path accordingly.
                 * 
                 * Special case: we need to artificially decrement the stored
                 * insertindex at the FINALSHAREDINDEX because the left
                 * continuation takes the left subtree of the primary range
                 * (whereas we took the right branch).
                 */
                if (pathindex == finalsharedindex) {
                    pathIndices[pathindex] = --insertindex;
                } else if (pathindex < finalsharedindex) {
                    struct ft_node* nextpathmember = path[pathindex + 1];
                    if (nextpathmember == right) {
                        pathIndices[pathindex] = ++insertindex;
                    }
                }

                if (topushnode != NULL) {
                    /* If this node is being split, then there are some
                     * complications. We need to change the node itself along
                     * the path. We also need to adjust the index accordingly.
                     */
                    if (insertindex <= FT_ORDER) {
                        path[pathindex] = subtree_get(&topushnode->subtrees[0]);
                    } else {
                        path[pathindex] = subtree_get(&topushnode->subtrees[1]);
                        pathIndices[pathindex] =
                            (insertindex -= (FT_ORDER + 1));
                    }
                }
            }

            if (topushnode == NULL) {
                // Nothing to push up
                /* Edge case: If we didn't hit the case where
                 * pathindex == finalsharedindex, we need to make sure that that
                 * entry of pathIndices got decremented anyway (so that the left
                 * continuation works as expected).
                 */
                if (rightcontinuation && pathindex > finalsharedindex) {
                    pathIndices[finalsharedindex]--;
                }
                return;
            }
            
            topushent = &topushnode->entries[0];
            left = subtree_get(&topushnode->subtrees[0]);
            right = subtree_get(&topushnode->subtrees[1]);
        }

        // No parent to push to
        this->root = topushnode;
        if (rightcontinuation) {
            struct ft_node* nextpathmember = path[0];
            memmove(&pathIndices[1], pathIndices,
                    args->pathIndices_len * sizeof(int));
            args->pathIndices_len++;
            memmove(&path[1], path, args->path_len * sizeof(struct ft_node*));
            args->path_len++;
            if (nextpathmember == subtree_get(&topushnode->subtrees[1])) {
                pathIndices[0] = 1;
            } else {
                ASSERT(nextpathmember == subtree_get(&topushnode->subtrees[0]),
                       "First element of path is not a child of the new root");
                pathIndices[0] = 0;
            }
            finalsharedindex++;
        }
    }
    if (rightcontinuation) {
        args->path_len = finalsharedindex;
        args->pathIndices_len = finalsharedindex;
    }
}

void ft_write(struct figtree* this, byte_index_t start, byte_index_t end,
              figtree_value_t value) {
    // Plus one because the height of the tree may increase on insert
    int maxdepth = this->root->HEIGHT + 1;
    struct ft_node* path[maxdepth];
    int pathIndices[maxdepth];
    struct insertargs iargs;
    struct insertcont starinserts;
    struct insertcont newstarinserts;

    i_init(&iargs.range, start, end);
    iargs.value = value;
    iargs.path = path;
    iargs.path_len = 0;
    iargs.pathIndices = pathIndices;
    iargs.pathIndices_len = 0;
    iargs.at = this->root;
    i_init(&iargs.valid, BYTE_INDEX_MIN, BYTE_INDEX_MAX);
    
    _ft_insert(this, &iargs, false, &starinserts);
    if (starinserts.hasrightc) {
        _ft_insert(this, &starinserts.rightc, true, &newstarinserts);
        ASSERT(!newstarinserts.hasleftc && !newstarinserts.hasrightc,
               "Recursive star insert on right continutation");
    }
    if (starinserts.hasleftc) {
        _ft_insert(this, &starinserts.leftc, false, &newstarinserts);
        ASSERT(!newstarinserts.hasleftc && !newstarinserts.hasrightc,
               "Recursive star insert on left continuation");
    }

    // Free the (aliased) lists of the continuations
    if (starinserts.hasleftc) {
        mem_free(starinserts.leftc.path);
        mem_free(starinserts.leftc.pathIndices);
    } else if (starinserts.hasrightc) {
        mem_free(starinserts.rightc.path);
        mem_free(starinserts.rightc.pathIndices);
    }
}

void ft_dealloc(struct figtree* this) {
    ftn_free(this->root);
    this->root = NULL;
}
