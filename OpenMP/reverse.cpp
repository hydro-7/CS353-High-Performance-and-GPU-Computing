#include <iostream>
#include <omp.h>
using namespace std;

#define N 100

int main() {
    int arr[N];

    for(int i = 0; i < N; i++)
        arr[i] = i + 1;

    #pragma omp parallel for
    for(int i = 0; i < N/2; i++) {
        int temp = arr[i];
        arr[i] = arr[N - i - 1];
        arr[N - i - 1] = temp;
    }

    for(int i = 0; i < N; i++)
        cout << arr[i] << " ";
}