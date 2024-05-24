#define _GNU_SOURCE

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>

#define CHECK 0     // FIXME:

typedef unsigned long long ad;

/* Opcode of request line */
typedef enum {
    ld, st, md
} opcode;

/* Result of cache access */
typedef enum {
    hit, miss, evict
} result;

/* A single cache block */
typedef struct {
    int valid;
    ad tag;
    int counter;
} block;

/* Program parameters */
static int s;           // number of set bits
static int b;           // number of block bits
static int E;           // set associativity
static char *filename;  // name of trace file

static void parse_arg(int argc, char **argv);
static int parse_line(char *line, opcode *oppt, ad *addrpt);
static void init_counter(block **cache, int nsets);
static result access(block **cache, ad addr, int nsets);

static int hash(ad addr, int nsets);
static void lru_count(block *set, int target);
static int find_match(block *set, ad tag);
static int find_empty(block *set);
static int find_victim(block *set);

static void check_set(block *set);


int main(int argc, char **argv)
{
    int hits = 0;
    int misses = 0;
    int evictions = 0;

    block **cache;          // cache as a 2D array of blocks
    int nsets;              // number of sets

    FILE *file;             // trace file object
    char *line;             // a line in file
    int len;                // length of a line
    size_t bufsize = 0;

    opcode op;              // opcode of each line
    ad addr;                // target address of each line
    result res;             // result of cache access

    int i;


    /* Parse program arguments */
    parse_arg(argc, argv);
    nsets = 1 << s;
    
    /* Allocate space to store cache info */
    cache = (block **)malloc(nsets * sizeof(block *));
    for (i=0; i<nsets; i++) {
        cache[i] = (block *)malloc(E * sizeof(block));
    }
    init_counter(cache, nsets);

    /* Open trace file */
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("File open error\n");
        exit(1);
    }

    /* Read lines of trace file */
    while ((len = getline(&line, &bufsize, file)) > 0) {
        /* Parse line */
        if (!parse_line(line, &op, &addr)) {
            continue;       // line starting with I
        }

        if (CHECK) {
            printf("\nline: %s\n", line);
        }

        /* Access the memory space with the given address */
        res = access(cache, addr, nsets);

        switch (res) {
        case hit:
            hits++;
            break;
        case miss:
            misses++;
            break;
        case evict:
            misses++;
            evictions++;
            break;
        default:
            printf("invalid result value\n");
            exit(1);
        }

        /* Double access */
        if (op == md) {
            hits++;
        }
    }
    fclose(file);

    /* Free spaces allocated for cache */
    for (i=0; i<nsets; i++) {
        free(cache[i]);
    }
    free(cache);

    /* Print results */
    printSummary(hits, misses, evictions);
    return 0;
}

/*
 * parse_arg - Parses programm argument.
 */
static void parse_arg(int argc, char **argv) {
    int i = 1;

    if (argc < 9) {
        printf("Too few arguments\n");
        exit(1);
    }

    while (i < argc) {
        if (argv[i][0] != '-') {
            printf("Invalid argument: %s\n", argv[i]);
            exit(1);
        }

        switch (argv[i][1]) {
        case 's':
            s = atoi(argv[i+1]);
            break;
        case 'b':
            b = atoi(argv[i+1]);
            break;
        case 'E':
            E = atoi(argv[i+1]);
            break;
        case 't':
            filename = argv[i+1];
            break;
        default:
            printf("Invalid argument: %s\n", argv[i]);
            exit(1);
        }

        i += 2;
    }
}

/* 
 * parse_line - Parses each line of trace files 
 *      to retrieve information about memory access.
 *      Returns 1 when succeeded, 0 when failed.
 */
static int parse_line(char *line, opcode *oppt, ad *addrpt) {
    char o;

    /* Do nothing for I operation */
    if (*line != ' ') {
        return 0;
    }
    line++;

    /* Get opcode */
    o = *line;
    switch (o) {
    case 'L':
        *oppt = ld;
        break;
    case 'S':
        *oppt = st;
        break;
    case 'M':
        *oppt = md;
        break;
    default:
        return 0;      // invalid opcode
    }
    line += 2;

    /* Get addr */
    *addrpt = (ad)strtol(line, NULL, 16);

    return 1;
}

/*
 * init_counter - Initializes LRU counter of every cache block,
 *      so that each block in a set has a distinct counter value.
 */
static void init_counter(block **cache, int nsets) {
    int i, j;

    for (i=0; i<nsets; i++) {
        for (j=0; j<E; j++) {
            cache[i][j].counter = j;
        }
    }
}

/*
 * access - Assuming the user is trying to access a memory space
 *      that belongs to the given set, modifies the cache contents
 *      properly and returns the result.
 */
static result access(block **cache, ad addr, int nsets) {
    int setno;          // target cache set
    block *set;
    ad tag;             // cache tag
    
    int idx;            // index inside a set

    /* Determines a set */
    setno = hash(addr, nsets);
    set = cache[setno];
    tag = addr >> (s + b);

    if (CHECK) {
        printf("setno %d tag %llx\n", setno, tag);
        check_set(set);
    }

    /* Case 1: Matching block found */
    if ((idx = find_match(set, tag)) >= 0) {
        lru_count(set, idx);    // increase counter value

        if (CHECK) {
            printf("\tHIT\n");
            check_set(set);
        }

        return hit;
    }

    /* Case 2: There is an empty space for a new block */
    if ((idx = find_empty(set)) >= 0) {
        /* Update block info */
        set[idx].valid = 1;
        set[idx].tag = tag;
        lru_count(set, idx);

        if (CHECK) {
            printf("\tMISS\n");
            check_set(set);
        }

        return miss;
    }

    /* Case 3: Should evict an old block */
    idx = find_victim(set);
    if (idx < 0) {
        printf("Cannot find victim\n");
        exit(1);
    }

    /* Update block info */
    set[idx].tag = tag;
    lru_count(set, idx);

    if (CHECK) {
        printf("\tEVICT\n");
        check_set(set);
    }

    return evict;
}

/*
 * hash - Determines to which set the given address belongs
 */
static int hash(ad addr, int nsets) {
    /* Remove lowest b bits */
    addr >>= b;

    /* Retrieve cache index */
    return addr % nsets;
}

/*
 * lru_count - Updates LRU counter values of blocks in given set
 */
static void lru_count(block *set, int target) {
    int i;

    for (i=0; i<E; i++) {
        if (i == target)
            continue;
        
        /* Push back blocks that were in front of the target block */
        if (set[i].counter > set[target].counter) {
            set[i].counter--;
        }
    }

    /* Set the target block to first */
    set[target].counter = E - 1;
}


/*
 * Functions to find a desire block in given set
 */
static int find_match(block *set, ad tag) {
    int i = 0;

    for (i=0; i<E; i++) {
        if (set[i].valid && set[i].tag == tag) {    // tag matches
            return i;
        }
    }
    return -1;
}
static int find_empty(block *set) {
    int i = 0;

    for (i=0; i<E; i++) {
        if (!set[i].valid) {
            return i;
        }
    }
    return -1;
}
static int find_victim(block *set) {
    int i = 0;

    for (i=0; i<E; i++) {
        if (set[i].counter == 0) {      // least recently used
            return i;
        }
    }
    return -1;
}

/*
 * check_set - Debugging tool. Prints contents of all blocks in given cache set.
 */
static void check_set(block *set) {
    int i;
    block bl;

    printf("**********\n");
    for (i=0; i<E; i++) {
        bl = set[i];
        printf("(%d) valid %d count %d tag %llx\n", i, bl.valid, bl.counter, bl.tag);
    }
}