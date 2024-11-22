/* 
 * 2014-17831 김재원
 * 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 * 
 * Assume s=5, E=1, b=5
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
// void printMatrices(int M, int N, int A[N][M], int B[M][N]);

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
    int i, j;                               
    int ii, jj;
    int b0, b1, b2, b3, b4, b5, b6, b7; 

    // Case 1: 32x32 matrix
    if(M == 32 && N == 32) {
        for(j = 0; j < N; j+=8) {
            for(i = 0; i < M; i++) {
                // store a block (8 ints) of values from a vertical axis of matrix A
                b0 = A[i][j];
                b1 = A[i][j+1];
                b2 = A[i][j+2];
                b3 = A[i][j+3];
                b4 = A[i][j+4];
                b5 = A[i][j+5];
                b6 = A[i][j+6];
                b7 = A[i][j+7];

                // assign the stored values to matrix B
                // all values should belong to the same block, hence no further evictions occur
                B[j][i] = b0;
                B[j+1][i] = b1;
                B[j+2][i] = b2;
                B[j+3][i] = b3;
                B[j+4][i] = b4;
                B[j+5][i] = b5;
                B[j+6][i] = b6;
                B[j+7][i] = b7;
            }
        }
    // Case 2: 64x64 matrix
    } else if (M == 64 && N == 64) {
        for(j = 0; j < N; j+=4) {
            for(i = 0; i < M; i+=8) {
                for(ii = 0; ii < 4; ii++) {
                    b0 = A[i][j+ii];
                    b1 = A[i+1][j+ii];
                    b2 = A[i+2][j+ii];
                    b3 = A[i+3][j+ii];
                    B[j+ii][i] = b0;
                    B[j+ii][i+1] = b1;
                    B[j+ii][i+2] = b2;
                    B[j+ii][i+3] = b3;
                }
                for(ii = 0; ii < 4; ii++) {
                    b4 = A[i+4][j+ii];
                    b5 = A[i+5][j+ii];
                    b6 = A[i+6][j+ii];
                    b7 = A[i+7][j+ii];
                    B[j+ii][i+4] = b4;
                    B[j+ii][i+5] = b5;
                    B[j+ii][i+6] = b6;
                    B[j+ii][i+7] = b7;
                }
            }
        }
    // Case 3: 61x67 matrix
    } else {
        for (i = 0; i < N; i += 18) {
            for (j = 0; j < M; j += 18) {
                // check (ii < N) and (jj < M) for corner cases, as 61 and 67 are not divisible by 18
                for (ii = i; ii < (i + 18) && ii < N; ii++) {
                    for (jj = j; jj < (j + 18) && jj < M; jj++) {
                        B[jj][ii] = A[ii][jj];
                    }
                }
            }
        }
    }

    // printMatrices(M, N, A, B);
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

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

/*
 * printMatrices(): prints matrices A and B.
 *
 * Used for debugging purposes ONLY:
 * (Calling this function during evaluation will increase number of misses significantly.)
 */
// void printMatrices(int M, int N, int A[N][M], int B[M][N])
// {
//     int i, j;

//     printf("\nA:\n");

//     for (i = 0; i < N; i++) {
//         for (j = 0; j < M; ++j) {
//             printf("%d ", A[i][j]);
//         }
//         printf("\n");
//     }

//     printf("\nB:\n");

//     for (i = 0; i < M; i++) {
//         for (j = 0; j < N; ++j) {
//             printf("%d ", B[i][j]);
//         }
//         printf("\n");
//     }
// }