#include <stdio.h>

void swap(int B[64][64], int x1, int y1, int x2, int y2) {
    int temp = B[x1][y1];
    B[x1][y1] = B[x2][y2];
    B[x2][y2] = temp;
}

int main() {
    int B[64][64];
    int i, j;

    for (i=0; i<64; i++) {
        for (j=0; j<64; j++) {
            B[i][j] = i+j;
        }
    }

    printf("%d %d\n", B[1][2], B[4][5]);
    
    swap(B, 1, 2, 4, 5);
    printf("%d %d\n", B[1][2], B[4][5]);

    return 0;
}