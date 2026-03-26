#include <iostream>
#include <omp.h>
using namespace std;

#define N 100

int main() {
    int arr[N];
    int key = 50;
    int found = 0;

    for(int i = 0; i < N; i++)
        arr[i] = i;

    #pragma omp parallel for
    for(int i = 0; i < N; i++) {
        if(arr[i] == key) {
            #pragma omp critical
            found = 1;
        }
    }

    if(found) cout << "Found\n";
    else cout << "Not Found\n";
}