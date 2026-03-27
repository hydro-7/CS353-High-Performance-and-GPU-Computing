#include <iostream>
#include <omp.h>
using namespace std;

#define N 8

int main() {
    int arr[N] = {1,2,3,4,5,6,7,8};
    int prefix[N];

    #pragma omp parallel for
    for(int i = 0; i < N; i++) {
        prefix[i] = 0;
        for(int j = 0; j <= i; j++) {
            prefix[i] += arr[j];
        }
    }

    for(int i = 0; i < N; i++)
        cout << prefix[i] << " ";
}