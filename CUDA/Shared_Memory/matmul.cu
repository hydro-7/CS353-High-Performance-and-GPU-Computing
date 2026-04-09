#include <stdio.h>

#define TILE_WIDTH 16

__global__ void matrixMul(int* A, int* B, int* C, int width) {
    // 1. Allocate 2D shared memory for tiles of A and B
    __shared__ int ds_A[TILE_WIDTH][TILE_WIDTH];
    __shared__ int ds_B[TILE_WIDTH][TILE_WIDTH];

    int bx = blockIdx.x;  int by = blockIdx.y;
    int tx = threadIdx.x; int ty = threadIdx.y;

    // Identify the row and column of the C element to work on
    int row = by * TILE_WIDTH + ty;
    int col = bx * TILE_WIDTH + tx;

    int pValue = 0;

    // Loop over the tiles required to compute the C element
    for (int p = 0; p < width / TILE_WIDTH; ++p) {
        
        // 2. Collaborative loading: each thread loads one element into the shared tiles
        ds_A[ty][tx] = A[row * width + (p * TILE_WIDTH + tx)];
        ds_B[ty][tx] = B[(p * TILE_WIDTH + ty) * width + col];
        __syncthreads(); // Ensure the whole tile is loaded

        // 3. Dot product using the fast shared memory tiles
        for (int i = 0; i < TILE_WIDTH; ++i) {
            pValue += ds_A[ty][i] * ds_B[i][tx];
        }
        __syncthreads(); // Ensure calculation is done before loading the next tile
    }

    // Write the final computed value to global memory
    C[row * width + col] = pValue;
}

// Launch configuration example:
// dim3 threadsPerBlock(TILE_WIDTH, TILE_WIDTH);
// dim3 blocksPerGrid(width / TILE_WIDTH, width / TILE_WIDTH);
// matrixMul<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, width);
