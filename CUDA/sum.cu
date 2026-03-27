#include <iostream>
using namespace std;

__global__ void sum(int *arr, int *result, int N) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;

    if(i < N) {
        atomicAdd(result, arr[i]);
    }
}

int main() {
    int N = 100;
    int arr[N], result = 0;

    for(int i = 0; i < N; i++) arr[i] = i + 1;

    int *d_arr, *d_result;

    cudaMalloc(&d_arr, N*sizeof(int));
    cudaMalloc(&d_result, sizeof(int));

    cudaMemcpy(d_arr, arr, N*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_result, &result, sizeof(int), cudaMemcpyHostToDevice);

    sum<<<(N+255)/256, 256>>>(d_arr, d_result, N);

    cudaMemcpy(&result, d_result, sizeof(int), cudaMemcpyDeviceToHost);

    cout << "Sum = " << result << endl;
}