#include <iostream>
using namespace std;

#define N 4

__global__ void matmul(int *A, int *B, int *C) {
    int row = threadIdx.y;
    int col = threadIdx.x;

    int sum = 0;
    for(int k = 0; k < N; k++) {
        sum += A[row*N + k] * B[k*N + col];
    }

    C[row*N + col] = sum;
}

int main() {
    int A[N][N], B[N][N], C[N][N];

    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++) {
            A[i][j] = i + j;
            B[i][j] = i * j;
        }

    int *d_A, *d_B, *d_C;

    cudaMalloc(&d_A, N*N*sizeof(int));
    cudaMalloc(&d_B, N*N*sizeof(int));
    cudaMalloc(&d_C, N*N*sizeof(int));

    cudaMemcpy(d_A, A, N*N*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, B, N*N*sizeof(int), cudaMemcpyHostToDevice);

    dim3 threads(N, N);
    matmul<<<1, threads>>>(d_A, d_B, d_C);

    cudaMemcpy(C, d_C, N*N*sizeof(int), cudaMemcpyDeviceToHost);

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++)
            cout << C[i][j] << " ";
        cout << endl;
    }
}