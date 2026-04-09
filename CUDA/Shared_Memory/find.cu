#include <stdio.h>

#define BLOCK_SIZE 256

__global__ void findElement(int* d_in, int target, int* d_result) {
    // Shared variable to hold a match found by this block
    __shared__ int block_result;

    // Initialize shared variable
    if (threadIdx.x == 0) {
        block_result = -1;
    }
    __syncthreads(); 

    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // Check for target. If found, write to shared memory.
    // Note: If multiple matches exist in this block, it's a race condition 
    // on who writes last, which is fine if we just want *any* index.
    if (d_in[i] == target) {
        block_result = i;
    }
    __syncthreads(); // Wait for all threads to finish checking

    // Thread 0 checks if anyone in the block found the target
    if (threadIdx.x == 0 && block_result != -1) {
        // Write the found index to global memory
        *d_result = block_result; 
    }
}

// Launch configuration example:
// findElement<<<blocks, threads>>>(d_in, 42, d_result);
