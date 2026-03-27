#include <iostream>
#include <omp.h>
using namespace std;

#define N 4

int main() {
    int A[N][N], B[N][N], C[N][N];

    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++) {
            A[i][j] = i + j;
            B[i][j] = i * j;
        }

    #pragma omp parallel for
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            C[i][j] = 0;
            for(int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++)
            cout << C[i][j] << " ";
        cout << endl;
    }
}