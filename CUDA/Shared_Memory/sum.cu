#include <stdio.h>

#define BLOCK_SIZE 256

__global__ void sumArray(int* d_in, int* d_out) {
    // 1. Allocate shared memory for this block
    __shared__ int sdata[BLOCK_SIZE];

    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // 2. Each thread loads one element from global to shared memory
    sdata[tid] = d_in[i];
    __syncthreads(); // Wait for all threads to finish loading

    // 3. Do reduction in shared memory (folding the array)
    for (int s = 1; s < blockDim.x; s *= 2) {
        if (tid % (2 * s) == 0) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads(); // Sync after each folding step
    }

    // 4. Thread 0 of this block writes the block's sum to the global output
    // We use atomicAdd in case we launched multiple blocks
    if (tid == 0) {
        atomicAdd(d_out, sdata[0]); 
    }
}

// Launch configuration example:
// int threads = BLOCK_SIZE;
// int blocks = (N + threads - 1) / threads;
// sumArray<<<blocks, threads>>>(d_in, d_total_sum);
