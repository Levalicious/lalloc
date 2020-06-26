#ifndef LALLOC_H
#define LALLOC_H

#define PAGESIZE (1048576)

#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>

typedef struct bblk bblock;
typedef bblock* bb;
struct bblk {
    size_t ind;
    bb next;
    size_t occ;
    char mem[PAGESIZE - (sizeof(size_t) + sizeof(bb) + sizeof(size_t))];
} __attribute__((packed));

typedef struct sblk sblock;
typedef sblock* sb;
struct sblk {
    sb prev;
    sb next;
    void* end;
    bb bblk;
    bool free;
} __attribute__((packed));

size_t bbhdr = (sizeof(size_t) + sizeof(bb) + sizeof(size_t));

bb first;

/**
 * @author Lev Knoblock
 * @notice Allocates a 'big block' of memory using mmap and inserts 1 'small block' into it
 * @dev Consider moving away from 1 page of memory. Maybe larger blocks would be better.
 * @param
 * @return bblk *
 */
bb bbinit() {
    bb out = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (out == MAP_FAILED) {
        printf("%s", sys_errlist[errno]);
        exit(40);
    }

    /* Big blocks are appended to an append-only linked list.
     * Since initially all memory in the block is free, the
     * occupancy is set to 0 */
    out->next = NULL;
    out->occ = 0;

    sb t = (sb) out->mem;
    /* The internal small block has no predecessors or
     * successors, but spans the full block width */
    t->prev = NULL;
    t->next = NULL;
    t->end = out->mem + (PAGESIZE - (sizeof(size_t) + sizeof(bb) + sizeof(size_t)));
    t->free = true;
    t->bblk = out;
    return out;
}

/**
 * @author Lev Knoblock
 * @notice Allocates a slice of memory by creating an appropriately sized small block in a big block
 * @dev Well its somehow slower than the prototype and I swear I knew what was making that one slow
 * @param 'big block' to allocate from, size of slice
 * @return void* to memory, or NULL if no space was found.
 */
static void* bblkalloc(bb blk, size_t size) {
    sb currSB = (sb) blk->mem;
    /* Find the first free small block */
    while (1) {
        if (currSB->free) break;
        tryGetNext:;
        if (currSB->next == NULL) {
            /* Reached end of big block */
            return NULL;
        }
        currSB = currSB->next;
    }

    /* Remaining space in small block */
    size_t frsize = currSB->end - (((void*)currSB) + sizeof(sblock));

    /* If there isn't enough space to fit a new small block
     * find another block that will fit one */
    if (frsize < (size + sizeof(sblock))) {
        goto tryGetNext;
    }

    /* Initialize the new small block by stealing
     * space from the end of the 'current' small block */
    sb newSB = currSB->end - (sizeof(sblock) + size);
    newSB->prev = currSB;
    newSB->next = currSB->next;
    newSB->end = currSB->end;
    newSB->free = false;
    newSB->bblk = blk;

    /* Append new small block to list */
    currSB->end = newSB;
    if (currSB->next != NULL) currSB->next->prev = newSB;
    currSB->next = newSB;

    currSB->bblk = blk;
    blk->occ++;
    /* Return pointer past allocation header */
    return ((void*)newSB) + sizeof(sblock);
}

/**
 * @author Lev Knoblock
 * @notice Allocates a slice of memory from the memory pool
 * @dev Currently has no functionality for reducing number of big blocks.
 * @param size of slice
 * @return void*
 */
void* lalloc(size_t size) {
    void* out;
    bb curr = first;
    unsigned int ind = 0;
    do {
        if (curr == NULL) {
            /* If current big block is null, set it up with its first small block */
            curr = bbinit();
            curr->ind = ind;
            if (ind == 0) first = curr;
        }
        /*
        if (curr->occ) {
            curr = curr->next;
            ind++;
            continue;
        }
         */
        out = bblkalloc(curr, size);
        /* If allocation fails go to the next big block (and allocate it if necessary)
         * otherwise, return the valid pointer */
        if (out != NULL) return out;
        //curr->occ = 1;
        curr = curr->next;
        ind++;
    } while (1);
}

/**
 * @author Lev Knoblock
 * @notice Frees a slice of memory from the memory pool
 * @dev Not really sure how to optimize further.
 * @param void* to slice
 * @return
 */
void lfree(void* a) {
    /* Decrement pointer to get to begining of header */
    sb currSB = a - sizeof(sblock);
    currSB->free = true;

    if (currSB->prev != NULL && currSB->prev->free) {
        /* If previous block exists and is free, extend it
         * to wrap over this one and remove pointers to
         * this block header */
        currSB->prev->end = currSB->end;
        currSB->prev->next = currSB->next;
        if (currSB->next != NULL) currSB->next->prev = currSB->prev;

        /* Replace block pointer on stack */
        currSB = currSB->prev;
    }

    if (currSB->next != NULL && currSB->next->free) {
        /* If next block exists extend current one over
         * it and scrub pointers to it */
        currSB->end = currSB->next->end;
        currSB->next = currSB->next->next;
        if (currSB->next != NULL) currSB->next->prev = currSB;
    }

    /* Decrement occupancy */
    currSB->bblk->occ--;
}

#endif
