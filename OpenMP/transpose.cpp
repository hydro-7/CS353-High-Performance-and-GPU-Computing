#include <iostream>
#include <omp.h>
using namespace std;

#define N 4

int main() {
    int A[N][N], B[N][N];

    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            A[i][j] = i * N + j;

    #pragma omp parallel for
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            B[j][i] = A[i][j];
        }
    }

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++)
            cout << B[i][j] << " ";
        cout << endl;
    }
}