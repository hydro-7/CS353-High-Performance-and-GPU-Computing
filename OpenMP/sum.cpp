#include <iostream>
#include <omp.h>
using namespace std;

#define N 100

int main() {
    int arr[N];
    int sum = 0;

    for(int i = 0; i < N; i++)
        arr[i] = i + 1;

    #pragma omp parallel for reduction(+:sum)
    for(int i = 0; i < N; i++) {
        sum += arr[i];
    }

    cout << "Sum = " << sum << endl;
}