#include <iostream>
using namespace std;

__global__ void prefix(int *arr, int *out, int N) {
    int i = threadIdx.x;

    int sum = 0;
    for(int j = 0; j <= i; j++) {
        sum += arr[j];
    }

    out[i] = sum;
}

int main() {
    int N = 8;
    int arr[8] = {1,2,3,4,5,6,7,8}, out[8];

    int *d_arr, *d_out;

    cudaMalloc(&d_arr, N*sizeof(int));
    cudaMalloc(&d_out, N*sizeof(int));

    cudaMemcpy(d_arr, arr, N*sizeof(int), cudaMemcpyHostToDevice);

    prefix<<<1, N>>>(d_arr, d_out, N);

    cudaMemcpy(out, d_out, N*sizeof(int), cudaMemcpyDeviceToHost);

    for(int i = 0; i < N; i++)
        cout << out[i] << " ";
}