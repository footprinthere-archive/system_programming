/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void transpose_32(int A[32][32], int B[32][32]);
void transpose_64(int A[64][64], int B[64][64]);
void transpose_6167(int A[67][61], int B[61][67]);
void swap(int B[64][64], int x1, int y1, int x2, int y2);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    /* 32x32 */
    if (M == 32 && N == 32)
        transpose_32(A, B);

    /* 64x64 */
    if (M == 64 && N == 64)
        transpose_64(A, B);

    /* 61x67 */
    if (M == 61 && N == 67)
        transpose_6167(A, B);

    else
        printf("Unexpected size\n");
}

/*
 * transpose_32 - Transpose function dedicated to 32x32 matrix.
 *      It divides each 8x8 block into four 4x4 parts to reduce the amount
 *      of conflicting miss.
 */
void transpose_32(int A[32][32], int B[32][32]) {
    int bx, by;     // starting point of each block
    int x, y;       // offset inside each block
    int temp;       // temp variable for swap

    for (bx=0; bx<32; bx+=8) {
        for (by=0; by<32; by+=8) {
            /* Lower left to upper right */
            for (x=7; x>=4; x--) {
                for (y=3; y>=0; y--) {
                    B[by+y][bx+x] = A[bx+x][by+y];
                }
            }

            /* Lower right to upper left (to minimize conflicts) */
            for (x=7; x>=4; x--) {
                for (y=4; y<=7; y++) {
                    B[by+y-4][bx+x-4] = A[bx+x][by+y];
                }
            }

            /* Upper right to lower left */
            for (x=0; x<=3; x++) {
                for (y=4; y<=7; y++) {
                    B[by+y][bx+x] = A[bx+x][by+y];
                }
            }

            /* Upper left to lower right (to minimize conflicts) */
            for (x=0; x<=3; x++) {
                for (y=0; y<=3; y++) {
                    B[by+y+4][bx+x+4] = A[bx+x][by+y];
                }
            }

            /* Swap inside B */
            for (x=0; x<=3; x++) {
                for (y=0; y<=3; y++) {
                    temp = B[by+x][bx+y];
                    B[by+x][bx+y] = B[by+x+4][bx+y+4];
                    B[by+x+4][bx+y+4] = temp;
                }
            }
        }
    }
}

/*
 * transpose_64 - Transpose function dedicated to 64x64 matrix.
 */
void transpose_64(int A[64][64], int B[64][64]) {
    int bx, by;
    int x, y;
    int offset;

    for (bx=0; bx<64; bx+=8) {
        for (by=0; by<64; by+=8) {
            /* For upper and lower half each */
            for (offset=0; offset<8; offset+=4) {
                /*** Move entries from A to B ***/
                /* A row 1, 2 -> B row 3, 4 (crossed to avoid conflicts) */
                for (x=offset; x<offset+2; x++) {
                    for (y=0; y<8; y++) {
                        B[by+x+2][bx+y] = A[bx+x][by+y];
                    }
                }

                /* A row 3, 4 -> B row 1, 2 */
                for (x=offset+2; x<offset+4; x++) {
                    for (y=0; y<8; y++) {
                        B[by+x-2][bx+y] = A[bx+x][by+y];
                    }
                }

                /*** Move entries inside B ***/
                /* Upper/Lower row 1, 2 <-> 3, 4 */
                for (x=offset; x<offset+2; x++) {
                    for (y=0; y<8; y++) {
                        swap(B, by+x, bx+y, by+x+2, bx+y);
                    }
                }

                /* Transpose upper/lower left part */
                for (x=offset; x<offset+4; x++) {
                    for (y=0; y<4; y++) {
                        if (x-offset > y) {
                            /* for lower triangular entries */
                            swap(B, by+x, bx+y, by+y+offset, bx+x-offset);
                        }
                    }
                }

                /* Transpose upper/lower right part */
                for (x=offset; x<offset+4; x++) {
                    for (y=4; y<8; y++) {
                        if (x-offset > y-4) {
                            swap(B, by+x, bx+y, by+y-4+offset, bx+x+4-offset);
                        }
                    }
                }

                /*
                 * Swap row 1, 2 <-> 3, 4 of upper right / lower left part
                 * (to avoid conflicts)
                 */
                for (x=offset+0; x<offset+2; x++) {
                    for (y=4-offset; y<8-offset; y++) {
                        swap(B, by+x, bx+y, by+x+2, bx+y);
                    }
                }
            }

            /*
             * Consider quadrants of each block as
             *      1 2
             *      3 4
             * 
             * Swap upper half of part 3 with lower half of part 2
             */
            for (x=4; x<6; x++) {
                for (y=0; y<4; y++) {
                    swap(B, by+x, bx+y, by+x-2, bx+y+4);
                }
            }

            /* Swap lower half of part 3 with upper half of part 2 */
            for (x=6; x<8; x++) {
                for (y=0; y<4; y++) {
                    swap(B, by+x, bx+y, by+x-6, bx+y+4);
                }
            }
        }
    }
}

/*
 * transpose_6167 - Transpose function dedicated to 61x67 matrix.
 *      It divides matrices into smaller blocks, and treats the
 *      diagonal entires separately.
 */
void transpose_6167(int A[67][61], int B[61][67]) {
    const int bsize=16;
    int bx, by;
    int x, y;
    int temp;

    for (bx=0; bx<61; bx+=bsize) {
        for (by=0; by<67; by+=bsize) {
            /* For each block */
            for (y=0; y<bsize && by+y<67; y++) {
                for (x=0; x<bsize && bx+x<61; x++) {
                    /* Diagonal entries */
                    if (bx+x == by+y) {
                        temp = A[by+y][by+y];
                    }
                    /* Other entries: Simple transpose */
                    else {
                        B[bx+x][by+y] = A[by+y][bx+x];
                    }
                }

                if (bx == by) {
                    B[by+y][by+y] = temp;
                }
            }
        }
    }
}

/*
 * swap - Changes to entries inside a 64x64 matrix
 */
void swap(int B[64][64], int x1, int y1, int x2, int y2) {
    int temp = B[x1][y1];
    B[x1][y1] = B[x2][y2];
    B[x2][y2] = temp;
}


/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

