#include <iostream>
using namespace std;

__global__ void reverse(int *arr, int N) {
    int i = threadIdx.x;

    if(i < N/2) {
        int temp = arr[i];
        arr[i] = arr[N-i-1];
        arr[N-i-1] = temp;
    }
}

int main() {
    int N = 100;
    int arr[N];

    for(int i = 0; i < N; i++) arr[i] = i + 1;

    int *d_arr;
    cudaMalloc(&d_arr, N*sizeof(int));

    cudaMemcpy(d_arr, arr, N*sizeof(int), cudaMemcpyHostToDevice);

    reverse<<<1, N>>>(d_arr, N);

    cudaMemcpy(arr, d_arr, N*sizeof(int), cudaMemcpyDeviceToHost);

    for(int i = 0; i < N; i++)
        cout << arr[i] << " ";
}