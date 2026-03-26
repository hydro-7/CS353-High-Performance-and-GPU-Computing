#include <iostream>
#include <pthread.h>
using namespace std;

#define N 4

int A[N][N], B[N][N];

void* transpose(void* arg) {
    int row = *(int*)arg;

    for(int j = 0; j < N; j++) {
        B[j][row] = A[row][j];
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[N];
    int rows[N];

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            A[i][j] = i * N + j;
        }
    }

    for(int i = 0; i < N; i++) {
        rows[i] = i;
        pthread_create(&threads[i], NULL, transpose, &rows[i]);
    }

    for(int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);

    cout << "Transpose:\n";
    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++)
            cout << B[i][j] << " ";
        cout << endl;
    }
}