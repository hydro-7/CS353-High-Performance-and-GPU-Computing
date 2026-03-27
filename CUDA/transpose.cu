#include <iostream>
using namespace std;

#define N 4

__global__ void transpose(int *A, int *B) {
    int i = threadIdx.y;
    int j = threadIdx.x;

    B[j*N + i] = A[i*N + j];
}

int main() {
    int A[N][N], B[N][N];

    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            A[i][j] = i*N + j;

    int *d_A, *d_B;

    cudaMalloc(&d_A, N*N*sizeof(int));
    cudaMalloc(&d_B, N*N*sizeof(int));

    cudaMemcpy(d_A, A, N*N*sizeof(int), cudaMemcpyHostToDevice);

    dim3 threads(N, N);
    transpose<<<1, threads>>>(d_A, d_B);

    cudaMemcpy(B, d_B, N*N*sizeof(int), cudaMemcpyDeviceToHost);

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++)
            cout << B[i][j] << " ";
        cout << endl;
    }
}