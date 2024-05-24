#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "memlib.h"

/* unit sizes */
#define WSIZE 4                 // size of a word
#define DSIZE 8                 // size of a double-word
#define CHUNKSIZE (1 << 12)     // size of a newly extended chunk

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* nubmer of segregated free lists */
#define NSETS 9

/* returns the greater number */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* packs a size and alloc bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* reads and writes a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* reads the size and alloc fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x7)

/* gets the address of the next link */
#define NEXTP(bp) ((char *)(bp) - 4)
#define GETNEXT(bp) ((void *)GET(NEXTP(bp)))
#define SETNEXT(bp, next) (PUT(NEXTP(bp), (addr)(next)))

/* computes address of its header and footer given block ptr */
#define HDRP(bp) ((char *)(bp) - 8)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 12)

/* computes address of next and previous blocks given block ptr */
#define NEXTBLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - 8)))
#define PREVFTRP(bp) ((char *)(bp) - 12)
#define PREVBLKP(bp) ((char *)(bp) - GET_SIZE(PREVFTRP(bp)))

typedef unsigned int addr;      // address value of pointer


void printw(char *msg, void *word) {
    printf("%s: size %d / alloc %d (%x)\n", msg, GET_SIZE(word), GET_ALLOC(word), GET(word));
}

void size_test() {
    void *ptr;

    mem_init();

    ptr = mem_sbrk(4*WSIZE);
    if (ptr == (void *)-1) {
        perror("sbrk error");
    }
    printf("init position %p\n", ptr);

    PUT(ptr, 0);
    PUT(ptr + 1*WSIZE, PACK(8, 1));
    PUT(ptr + 2*WSIZE, PACK(8, 1));
    PUT(ptr + 3*WSIZE, PACK(0, 1));
    ptr += 2*WSIZE;
    printf("first block addr: %p\n", ptr);
    printw("header: ", HDRP(ptr));
    printf("\theader addr: %p\n", HDRP(ptr));
    printw("footer: ", FTRP(ptr));
    printf("\tfooter addr: %p\n", FTRP(ptr));
    printw("epilogue: ", HDRP(NEXTBLKP(ptr)));
    printf("\tepilogue addr: %p\n", HDRP(NEXTBLKP(ptr)));
    printf("******\n");
}

// void nextlink_test() {
//     char *heappt;
//     int i;

//     int n = 3;
//     void *pt = (void *)(&n);

//     mem_init();

//     /* Create the initial empty heap */
//     heappt = (char *)mem_sbrk(7*DSIZE);
//     if (heappt == (void *)-1) {
//         return;
//     }

//     /* Put the root links of free lists */
//     for (i=0; i<=8; i++) {
//         PUT(heappt, 0);   // initially null
//         heappt += WSIZE;
//     }

//     /* Put the prologue & epilogue blocks */
//     PUT(heappt, PACK(8, 1));                // prologue header
//     PUT(heappt + 1*WSIZE, (unsigned int)pt);
//     PUT(heappt + 2*WSIZE, 0);               // prologue padding
//     PUT(heappt + 3*WSIZE, PACK(8, 1));      // prologue footer
//     PUT(heappt + 4*WSIZE, PACK(0, 1));      // epilogue header
//     heappt += 2*WSIZE;

//     printf("%p\n", pt);
//     printf("%p\n", NEXTLINK(heappt));
// }

void addnext_test() {
    unsigned int *root = (unsigned int *)malloc(WSIZE);
    void *first = malloc(4*WSIZE);
    void *new = malloc(4*WSIZE);
    void *block;

    PUT(first, PACK(16, 0));
    PUT(first + 1*WSIZE, 0);
    PUT(first + 2*WSIZE, 10);
    PUT(first + 3*WSIZE, PACK(16, 0));
    first += 2*WSIZE;
    *root = (unsigned int)first;
    
    PUT(new, PACK(16, 0));
    PUT(new + 1*WSIZE, 0);
    PUT(new + 2*WSIZE, 22);
    PUT(new + 3*WSIZE, PACK(16, 0));
    new += 2*WSIZE;

    block = (void *)(*root);
    printf("%d\n", *(int *)block);

    SETNEXT(new, first);
    *root = (unsigned int)new;

    block = (void *)(*root);
    printf("%d\n", *(int *)block);
    block = GETNEXT(block);
    printf("%d\n", *(int *)block);
}

void addpop_test() {
    addr *root = (addr *)malloc(WSIZE);
    void *first = malloc(4*WSIZE);
    void *new = malloc(4*WSIZE);
    void *prev;
    void *block;
    void *target;

    PUT(first, PACK(16, 0));
    PUT(first + 1*WSIZE, 0);
    PUT(first + 2*WSIZE, 10);           // val 10
    PUT(first + 3*WSIZE, PACK(16, 0));
    first += 2*WSIZE;
    printf("first : %p\n", first);
    *root = (addr)first;
    printf("root : %p\n", (void *)(*root));
    
    PUT(new, PACK(16, 0));
    PUT(new + 1*WSIZE, 0);
    PUT(new + 2*WSIZE, 22);             // val 22
    PUT(new + 3*WSIZE, PACK(16, 0));
    new += 2*WSIZE;
    printf("new : %p\n", new);

    /* add */
    SETNEXT(new, first);
    *root = (addr)new;

    block = (void *)(*root);
    printf("%d\n", *(int *)block);
    block = GETNEXT(block);
    printf("%d\n", *(int *)block);
    printf("******\n");

    /* pop */
    target = first;
    printf("target : %p\n", target);

    prev = (void *)(*root);
    if (prev == NULL) {
        printf("empty\n");
        return;
    }

    if (prev == target) {
        printf("first block matched\n");
        block = GETNEXT(prev);
        *root = (addr)block;
        printf("now root : %p\n", (void *)(*root));
        goto CHECK;
    }

    while ((block = GETNEXT(prev)) != target) {
        if (block == NULL) {
            printf("nout found\n");
            return;
        }
        prev = block;
    }

    SETNEXT(prev, GETNEXT(block));
    printf("set %p -> next %p\n", prev, GETNEXT(block));

CHECK:
    block = (void *)(*root);
    printf("%d\n", *(int *)block);
    block = GETNEXT(block);
    if (!block)
        printf("NULL\n");
    else
        printf("%d\n", *(int *)block);
}

int main() {
    addpop_test();

    return 0;
}
