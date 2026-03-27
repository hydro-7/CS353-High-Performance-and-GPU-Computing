#include <iostream>
using namespace std;

__global__ void search(int *arr, int key, int *found, int N) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;

    if(i < N && arr[i] == key) {
        *found = 1;
    }
}

int main() {
    int N = 100;
    int arr[N], key = 50, found = 0;

    for(int i = 0; i < N; i++) arr[i] = i;

    int *d_arr, *d_found;

    cudaMalloc(&d_arr, N * sizeof(int));
    cudaMalloc(&d_found, sizeof(int));

    cudaMemcpy(d_arr, arr, N * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_found, &found, sizeof(int), cudaMemcpyHostToDevice);

    search<<<(N+255)/256, 256>>>(d_arr, key, d_found, N);

    cudaMemcpy(&found, d_found, sizeof(int), cudaMemcpyDeviceToHost);

    if(found) cout << "Found\n";
    else cout << "Not Found\n";
}