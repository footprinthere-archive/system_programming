/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * FIXME:
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
#define CHUNKSIZE (1 << 12)     // size of a newly extended chunk

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

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

/* computes address of its header and footer given block ptr */
#define HDRP(bp) ((char *)(bp) - 4)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 8)

/* computes address of next and previous blocks given block ptr */
#define NEXTBLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - 4)))
#define PREVBLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - 8)))
#define PREVFTRP(bp) ((char *)(bp) - 8)

#define CHECK 0     // FIXME: remove all lines containing this macro
#define OFFSET(bp) ((char *)(bp) - (char *)(mem_heap_lo()) - 8)

static void *extend_heap(size_t words);
static void *coalesce(void *ptr);
static void *find_fit(size_t size);
static void place(void *ptr, size_t size);

static void printw(char *msg, char *word);
static void mm_check_init();
static void mm_check_implicit();


/* 
 * mm_init - Initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    void *heap_ptr = mem_sbrk(4*WSIZE);
    if (heap_ptr == (void *)-1) {
        return -1;
    }

    /* Put the initial values */
    PUT(heap_ptr, 0);                       // alignment padding
    PUT(heap_ptr + 1*WSIZE, PACK(8, 1));    // prologue header
    PUT(heap_ptr + 2*WSIZE, PACK(8, 1));    // prologue footer
    PUT(heap_ptr + 3*WSIZE, PACK(0, 1));    // epilogue header
    heap_ptr += 2*WSIZE;                    // move to point the prologue block
    
    if (CHECK) mm_check_init();

    /* Extend the empty heap with a free block */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }

    if (CHECK) mm_check_implicit();
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize;         // adjusted size
    size_t extendsize;      // amount to extend heap
    char *ptr;

    if (CHECK) printf("malloc(%d) called\n", size);

    /* Ignore when size=0 */
    if (size == 0) {
        return NULL;
    }

    /* Adjust block size to meet the alignment condition */
    if (size <= 2*WSIZE) {
        newsize = 4*WSIZE;                  // minimum block size of 4 words
    } else {
        newsize = ALIGN(size + 2*WSIZE);    // 2 words added for header and footer
    }

    /* Find an appropriate space to allocate */
    if ((ptr = find_fit(newsize)) != NULL) {
        place(ptr, newsize);

        if (CHECK) printf("-> malloc(%d) found fit space\n", size);
        if (CHECK) mm_check_implicit();

        return ptr;
    }

    /* No fit space; Extend heap to get more space */
    extendsize = MAX(newsize, CHUNKSIZE);
    if ((ptr = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    place(ptr, newsize);

    if (CHECK) printf("-> malloc(%d) extended heap\n", size);
    if (CHECK) mm_check_implicit();

    return ptr;
}

/*
 * mm_free - Free the block in the given position.
 *      Behaviour defined only when the position was allocated
 *      using malloc() or realloc().
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));  // retrieve block size info

    /* Reset alloc bits of header and footer */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);      // coalesce if necessary

    if (CHECK) printf("free (size=%d)\n", size);
    if (CHECK) mm_check_implicit();
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/* HELPER ROUTINES */

/*
 * extend_heap - Extend the empty heap by the given size.
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
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXTBLKP(bp)), PACK(0, 1));
    
    /* Coalesce if the previous block was free */
    bp = coalesce(bp);
    return bp;
}

/*
 * coalesce - Merge neighboring free blocks.
 */
static void *coalesce(void* ptr) {
    size_t prev_alloc = GET_ALLOC(PREVFTRP(ptr));
    size_t next_alloc = GET_ALLOC(HDRP(NEXTBLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    /* Case 1: No neighboring free blocks */
    if (prev_alloc && next_alloc) {
        return ptr;
    }

    /* Case 2: Previous block free */
    if (!prev_alloc && next_alloc) {
        size += GET_SIZE(PREVFTRP(ptr));            // calculate new block size
        PUT(FTRP(ptr), PACK(size, 0));              // update footer
        PUT(HDRP(PREVBLKP(ptr)), PACK(size, 0));    // update header
        return PREVBLKP(ptr);
    }

    /* Case 3: Next block free */
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXTBLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(NEXTBLKP(ptr)), PACK(size, 0));
        return ptr;
    }

    /* Case 4: Both previous and next blocks free */
    else {
        size += GET_SIZE(PREVFTRP(ptr)) + GET_SIZE(HDRP(NEXTBLKP(ptr)));
        PUT(HDRP(PREVBLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXTBLKP(ptr)), PACK(size, 0));
        return PREVBLKP(ptr);
    }
}

/*
 * find_fit - Find an appropriate space to allocate among free blocks.
 *      Return NULL when there is no fit space
 */
static void *find_fit(size_t size) {
    char * const first_block = (char *)mem_heap_lo() + 2*WSIZE;
    char *blockpt = first_block;
    size_t alloc;
    size_t blocksize;

    while ((blocksize = GET_SIZE(HDRP(blockpt))) > 0) {
        alloc = GET_ALLOC(HDRP(blockpt));
        if (!alloc && blocksize >= size) {      // free and enough size
            if (CHECK) printf("\tfind_fit() returns a space at %d\n", OFFSET(blockpt));
            return blockpt;
        }

        /* Move to the next block */
        blockpt = NEXTBLKP(blockpt);
    }

    /* Cannot find an appropriate space */
    return NULL;
}

/*
 * place - Allocate a space of the given size at the given position,
 *      and perform split if necessary
 */
static void place(void *ptr, size_t size) {
    size_t blocksize = GET_SIZE(HDRP(ptr));
    size_t rest = blocksize - size;     // size of remaining space

    PUT(HDRP(ptr), PACK(size, 1));      // update header
    PUT(FTRP(ptr), PACK(size, 1));      // update footer
    if (rest == 0) {
        return;                         // exactly fit
    }

    /* Split the free block */
    PUT(HDRP(NEXTBLKP(ptr)), PACK(rest, 0));    // update header of the remaining block
    PUT(FTRP(NEXTBLKP(ptr)), PACK(rest, 0));    // update footer of the remaining block
}


/*
 * Debugging tools
 */

static void printw(char *msg, char *word) {
    printf("%s: size %d / alloc %d (%x)\n", msg, GET_SIZE(word), GET_ALLOC(word), GET(word));
}

static void mm_check_init() {
    void *ptr = mem_heap_lo();
    
    printf("*** mm_check_init ***\n");
    printw("padding: ", ptr);
    printf("\theap lo addr: %p\n", ptr);

    ptr += 2*WSIZE;
    printf("first block addr: %p\n", ptr);
    printw("header: ", HDRP(ptr));
    printf("\theader addr: %p\n", HDRP(ptr));
    printw("footer: ", FTRP(ptr));
    printf("\tfooter addr: %p\n", FTRP(ptr));
    printw("epilogue: ", HDRP(NEXTBLKP(ptr)));
    printf("\tepilogue addr: %p\n", HDRP(NEXTBLKP(ptr)));
    printf("******\n\n");
}

static void mm_check_implicit() {
    char * const first_block = (char *)mem_heap_lo() + 2*WSIZE;
    char * const heap_end = (char *)mem_heap_hi();
    
    char *ptr = first_block;
    size_t alloc;
    size_t blocksize;

    printf("*** mm_check_implicit ***\n");
    printf("heap %p ~ %p\n", first_block, heap_end);
    printf("\tcurrent heap size %d\n", heap_end - first_block + 1 + 2*WSIZE);

    while ((blocksize = GET_SIZE(HDRP(ptr))) > 0) {
        alloc = GET_ALLOC(HDRP(ptr));
        if (alloc) {
            printf("block : alloc / size %d\n", blocksize);
        } else {
            printf("block : free / size %d\n", blocksize);
        }

        ptr = NEXTBLKP(ptr);
    }

    printf("******\n\n");
}
