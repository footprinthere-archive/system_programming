/*
 * mm.c
 *
 * Implement memory allocation of C, using segregated free lists scheme.
 * Supports malloc, realloc and free.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* unit sizes */
#define WSIZE 4                 // size of a word
#define DSIZE 8                 // size of a double-word
#define CHUNKSIZE (1 << 12)     // size of a newly extended chunk

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* nubmer of segregated free lists */
#define NSETS 15                // must be an odd number

/* returns the greater number */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* packs a size and alloc bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* reads and writes a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* reads the size and alloc fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* gets the address of the next link */
#define GETNEXT(bp) ((void *)GET(bp))
#define SETNEXT(bp, next) (PUT(bp, (addr)(next)))

/* computes address of its header and footer given block ptr */
#define HDRP(bp) ((char *)(bp) - 4)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 8)

/* computes address of next and previous blocks given block ptr */
#define NEXTBLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREVFTRP(bp) ((char *)(bp) - 8)
#define PREVBLKP(bp) ((char *)(bp) - GET_SIZE(PREVFTRP(bp)))

typedef unsigned int addr;      // address value of pointer

/* Helper routines */

static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void *find_fit(size_t size);
static void place(void *ptr, size_t size, int was_free);
static void increase(void *ptr, size_t size);

/* Segregated list management tools */

static void add_block(void *block, size_t size);
static void pop_block(void *block, size_t size);
static int which_set(size_t size);
static void add(int setno, void *block);
static void pop(int setno, void *block);

/* Debugging tools */

static void printw(char *msg, char *word);
static void mm_check_init();
static void mm_check_heap();
static void mm_check_segregated();


/* 
 * mm_init - Initializes the malloc package.
 */
int mm_init(void)
{
    char *heappt;
    int i;

    /* Create the initial empty heap */
    heappt = (char *)mem_sbrk((NSETS+5)*WSIZE);
    if (heappt == (void *)-1) {
        return -1;
    }

    /* Put the root links of free lists */
    for (i=0; i<NSETS; i++) {
        PUT(heappt, 0);   // initially null
        heappt += WSIZE;
    }

    /* Put the prologue & epilogue blocks */
    PUT(heappt, PACK(16, 1));               // prologue header
    PUT(heappt + 1*WSIZE, 0);               // prologue payload
    PUT(heappt + 2*WSIZE, 0);               // prologue payload
    PUT(heappt + 3*WSIZE, PACK(16, 1));     // prologue footer
    PUT(heappt + 4*WSIZE, PACK(0, 1));      // epilogue header

    /* Extend the empty heap with a large free block */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Searches for an appropriate block and allocate it to user.
 *     Always allocates a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize;         // adjusted block size
    size_t extendsize;      // amount to extend heap
    void *ptr;

    /* Ignore when size=0 */
    if (size == 0) {
        return NULL;
    }

    /* Adjust block size to meet the alignment condition */
    newsize = ALIGN(size + 2*WSIZE);    // minimum block size: 4 words

    /* Find an appropriate space to allocate */
    if ((ptr = find_fit(newsize)) != NULL) {
        place(ptr, newsize, 1);
        return ptr;
    }

    /* No fit space; Extend heap to get more space */
    extendsize = MAX(newsize, CHUNKSIZE);
    if ((ptr = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place(ptr, newsize, 1);

    return ptr;
}

/*
 * mm_free - Frees the block in the given position.
 *      Behaviour defined only when the position was allocated
 *      using malloc() or realloc().
 */
void mm_free(void *ptr)
{
    size_t size;

    /* Retrieve block size info */
    size = GET_SIZE(HDRP(ptr));

    /* Reset alloc bits of header and footer */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    /* Coalesce and add to free list */
    coalesce(ptr);
}

/*
 * mm_realloc - Increases or decreases the size of given block,
 *      or finds another appropriate block depding on situations.
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t newsize;             // adjusted block size
    size_t orig_size;           // size of the originally allocated block
    size_t nextblock_size = 0;  // size of the next free block

    void *newptr;               // pointer to a newly allocated block

    /* Trivial cases */
    if (ptr == NULL) {          // call malloc if ptr is NULL
        return mm_malloc(size);
    }
    if (size == 0) {            // call free if size is 0
        mm_free(ptr);
        return NULL;
    }

    orig_size = GET_SIZE(HDRP(ptr));

    /* Adjust block size to meet the alignment condition */
    newsize = ALIGN(size + 2*WSIZE);

    /* Case 1: Same size */
    if (newsize == orig_size) {
        return ptr;                     // do nothing
    }

    /* Case 2: Decreased size */
    else if (newsize < orig_size) {
        place(ptr, newsize, 0);         // split the original block
        return ptr;
    }

    /* Case 3: Increased size */

    /* Case 3-1: Can merge with the next block to get enough size */
    if (!GET_ALLOC(HDRP(NEXTBLKP(ptr)))) {
        nextblock_size = GET_SIZE(HDRP(NEXTBLKP(ptr)));
        
        if (orig_size + nextblock_size >= newsize) {
            increase(ptr, newsize);
            return ptr;
        }
    }
    
    /* Case 3-2: Need to find a new space */
    newptr = mm_malloc(size);           // allocate a new block
    memmove(newptr, ptr, size);         // copy the contents of the original block
    mm_free(ptr);
    return newptr;
}

/****** HELPER ROUTINES ******/

/*
 * extend_heap - Extends the empty heap by the given size.
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even nubmer of words */
    size = (words % 2) ? (words + 1)*WSIZE : words*WSIZE;
    bp = mem_sbrk(size);
    if (bp == (void *)-1) {
        return NULL;
    }

    /* Initialize free block and epilogue header */
    PUT(HDRP(bp), PACK(size, 0));           // overwrite epilogue header
    SETNEXT(bp, NULL);
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXTBLKP(bp)), PACK(0, 1));    // new epilogue header
    
    /* Coalesce if the previous block was free */
    bp = coalesce(bp);
    return bp;
}

/*
 * coalesce - Merges neighboring free blocks,
 *      and puts the new block to the appropriate free list.
 */
static void *coalesce(void* ptr) {
    size_t prev_alloc = GET_ALLOC(PREVFTRP(ptr));
    size_t next_alloc = GET_ALLOC(HDRP(NEXTBLKP(ptr)));
    size_t prev_size;
    size_t next_size;
    size_t size = GET_SIZE(HDRP(ptr));

    /* Case 1: No neighboring free blocks */
    if (prev_alloc && next_alloc) {
        add_block(ptr, size);                       // add to free list
    }

    /* Case 2: Previous block free */
    else if (!prev_alloc && next_alloc) {
        prev_size = GET_SIZE(HDRP(PREVBLKP(ptr)));
        pop_block(PREVBLKP(ptr), prev_size);        // pop previous block from free list

        size += prev_size;                          // calculate new block size
        PUT(FTRP(ptr), PACK(size, 0));              // update footer
        PUT(HDRP(PREVBLKP(ptr)), PACK(size, 0));    // update header

        ptr = PREVBLKP(ptr);
        add_block(ptr, size);                       // add new block to free list
    }

    /* Case 3: Next block free */
    else if (prev_alloc && !next_alloc) {
        next_size = GET_SIZE(HDRP(NEXTBLKP(ptr)));
        pop_block(NEXTBLKP(ptr), next_size);
        
        size += next_size;
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));

        add_block(ptr, size);
    }

    /* Case 4: Both previous and next blocks free */
    else {
        prev_size = GET_SIZE(PREVFTRP(ptr));
        pop_block(PREVBLKP(ptr), prev_size);

        next_size = GET_SIZE(HDRP(NEXTBLKP(ptr)));
        pop_block(NEXTBLKP(ptr), next_size);

        size += prev_size + next_size;
        PUT(HDRP(PREVBLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXTBLKP(ptr)), PACK(size, 0));

        ptr = PREVBLKP(ptr);
        add_block(ptr, size);
    }

    return ptr;
}

/*
 * find_fit - Finds an appropriate space to allocate among free blocks.
 *      Returns NULL when there is no fit space.
 */
static void *find_fit(size_t size) {
    int setno = which_set(size);
    addr *root;
    void *blockpt;

    while (setno < NSETS) {
        /* Get the first block of each list */
        root = (addr *)mem_heap_lo() + setno;
        blockpt = (void *)(*root);

        /* Check each block in the list */
        while (blockpt) {
            if (GET_SIZE(HDRP(blockpt)) >= size) {
                break;      // appropriate block found
            }
            blockpt = GETNEXT(blockpt);
        }
        if (blockpt) {
            break;          // found
        }
        setno++;            // move to the next list
    }

    return blockpt;         // NULL when not found
}

/*
 * place - Allocates a space of the given size in the given block,
 *      and performs split if necessary.
 */
static void place(void *ptr, size_t size, int was_free) {
    size_t blocksize = GET_SIZE(HDRP(ptr));
    size_t rest = blocksize - size;

    if (was_free) {
        pop_block(ptr, blocksize);      // pop original block
    }

    if (rest < 4*WSIZE) {               // remaining space is too small
        size = blocksize;               // give the entire block
        rest = 0;
    }
    
    PUT(HDRP(ptr), PACK(size, 1));      // update header
    PUT(FTRP(ptr), PACK(size, 1));      // update footer
    if (rest == 0) {
        return;                         // exactly fit
    }
    
    /* Handle the remaining block */
    ptr = NEXTBLKP(ptr);
    PUT(HDRP(ptr), PACK(rest, 0));      // update header
    PUT(FTRP(ptr), PACK(rest, 0));      // update footer
    add_block(ptr, rest);               // add to free list
}

/*
 * increase - Merges the given block with the following free block
 *      to get more space.
 */
static void increase(void *ptr, size_t size) {
    size_t block_size = GET_SIZE(HDRP(ptr));
    size_t nextblock_size = GET_SIZE(HDRP(NEXTBLKP(ptr)));
    size_t rest = (block_size + nextblock_size) - size;

    if (rest < 4*WSIZE) {               // remaining space is too small
        size = block_size + nextblock_size;
        rest = 0;
    }

    /* Pop the following block from free list */
    pop_block(NEXTBLKP(ptr), nextblock_size);

    /* Update size info */
    PUT(HDRP(ptr), PACK(size, 1));      // update header
    PUT(FTRP(ptr), PACK(size, 1));      // update footer
    if (rest == 0) {
        return;                         // exactly fit
    }

    /* Handle the remaining block */
    ptr = NEXTBLKP(ptr);
    PUT(HDRP(ptr), PACK(rest, 0));      // update header
    PUT(FTRP(ptr), PACK(rest, 0));      // update footer
    add_block(ptr, rest);               // add to free list
}

/****** LIST MANAGEMENT TOOLS ******/

/*
 * add_block - Adds the given block to the appropriate set.
 */
static void add_block(void *block, size_t size) {
    int setno = which_set(size);
    add(setno, block);
}

/*
 * pop_block - Removes the given block from the appropriate set .
 */
static void pop_block(void *block, size_t size) {
    if (size == 0) {
        printf("pop_block() called with size=0\n");
        exit(1);
    }

    int setno = which_set(size);
    pop(setno, block);
}

/*
 * which_set - Given a requested size, determines to which set
 *      the new block should belong.
 */
static int which_set(size_t size) {
    int setno = 0;
    int limit = 32;

    if (size == 0) {
        printf("which_set() called for size 0\n");
        exit(1);
    }

    while (setno < NSETS-1) {
        if (size <= limit) {
            return setno;
        }

        setno++;
        limit <<= 1;        // limit = 2 ^ (setno + 5)
    }
    return setno;           // the last set
}

/*
 * add - Adds the new block to the specified set.
 */
static void add(int setno, void *block) {
    addr *root;
    void *first;

    /* Get the address of the first block */
    root = (addr *)mem_heap_lo() + setno;
    first = (void *)(*root);
    
    /* Insert the new block */
    SETNEXT(block, first);      // set NULL if the list was empty
    *root = (addr)block;
}

/*
 * pop - Removes the block from the specified set.
 */
static void pop(int setno, void *block) {
    addr *root;
    void *prev;
    void *next;

    /* Get the address of the first block */
    root = (addr *)mem_heap_lo() + setno;
    prev = (void *)(*root);
    if (prev == NULL) {                 // empty list
        printf("pop(): called for an empty set\n");
        exit(1);
    }

    /* Find the requested block in the list */
    if (prev == block) {                // first block matched
        next = GETNEXT(prev);
        *root = (addr)next;
        return;
    }
    
    while ((next = GETNEXT(prev)) != block) {
        if (next == NULL) {             // not found
            printf("pop(): requested block not found\n");
            exit(1);
        }
        prev = next;
    }

    /* Remove the block */
    SETNEXT(prev, GETNEXT(next));
}


/****** DEBUGGING TOOLS ******/

/*
 * printw - Prints content of the given word
 */
static void printw(char *msg, char *word) {
    printf("%s: size %d / alloc %d (%x)\n", msg, GET_SIZE(word), GET_ALLOC(word), GET(word));
}

/*
 * mm_check_init - Checks if the initialization by mm_init was done well
 */
static void mm_check_init() {
    void *ptr = mem_heap_lo();
    
    printf("*** check INIT ***\n");
    printf("\theap lo addr: %p\n", ptr);

    /* Check each section of init blocks */
    ptr += 10*WSIZE;
    printf("first block addr: %p\n", ptr);
    printw("header: ", HDRP(ptr));
    printf("\theader addr: %p\n", HDRP(ptr));
    printf("next: %p\n", GETNEXT(ptr));
    printw("footer: ", FTRP(ptr));
    printf("\tfooter addr: %p\n", FTRP(ptr));
    printw("epilogue: ", HDRP(NEXTBLKP(ptr)));
    printf("\tepilogue addr: %p\n", HDRP(NEXTBLKP(ptr)));
    printf("******\n\n");
}

/*
 * mm_check_heap - Checks the contents of all blocks in the heap
 */
static void mm_check_heap() {
    char * const first_block = (char *)mem_heap_lo() + 10*WSIZE;
    char * const heap_end = (char *)mem_heap_hi();
    
    char *ptr = first_block;
    size_t alloc;
    size_t blocksize;

    printf("*** check HEAP ***\n");
    printf("heap %p ~ %p\n", first_block, heap_end);
    printf("\tcurrent heap size %d\n", heap_end - first_block + 1 + 2*WSIZE);

    /* Probe each block */
    while ((blocksize = GET_SIZE(HDRP(ptr))) > 0) {
        alloc = GET_ALLOC(HDRP(ptr));
        if (alloc) {
            printf("block : alloc / size %d\n", blocksize);
        } else {
            printf("block : free / size %d\n", blocksize);
        }

        /* Detect header-footer mismatch */
        if (blocksize != GET_SIZE(FTRP(ptr))) {
            printf("@@@@@ header-footer size info does not match\n");
            exit(1);
        }
        if (alloc != GET_ALLOC(FTRP(ptr))) {
            printf("@@@@@ header-footer alloc info does not match\n");
            exit(1);
        }

        ptr = NEXTBLKP(ptr);
    }

    printf("******\n\n");
}

/*
 * mm_check_segregated - Checks the contents of all size sets
 *      of the segregated free list
 */
static void mm_check_segregated() {
    int setno = 0;
    addr *root;
    void *ptr;

    printf("*** check SEGREGATED ***\n");
    
    /* Probe each size set */
    while (setno < NSETS) {
        printf("-- set %d\n", setno);
        root = (addr *)mem_heap_lo() + setno;
        ptr = (void *)(*root);

        while (ptr) {
            printw("\t", HDRP(ptr));
            ptr = GETNEXT(ptr);
        }
        setno++;
    }
    printf("******\n\n");
}
