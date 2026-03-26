#include <iostream>
#include <pthread.h>
using namespace std;

#define N 4

int A[N][N], B[N][N], C[N][N];

void* multiply(void* arg) {
    int row = *(int*)arg;

    for(int j = 0; j < N; j++) {
        C[row][j] = 0;
        for(int k = 0; k < N; k++) {
            C[row][j] += A[row][k] * B[k][j];
        }
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[N];
    int rows[N];

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            A[i][j] = i + j;
            B[i][j] = i * j;
        }
    }

    for(int i = 0; i < N; i++) {
        rows[i] = i;
        pthread_create(&threads[i], NULL, multiply, &rows[i]);
    }

    for(int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);

    cout << "Result Matrix:\n";
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++)
            cout << C[i][j] << " ";
        cout << endl;
    }
}