#include <iostream>
#include <pthread.h>
using namespace std;

#define N 100

int arr[N];
int num_threads = 4;

void* reverse_array(void* arg) {
    int id = *(int*)arg;
    int chunk = (N / 2) / num_threads;

    int start = id * chunk;
    int end = start + chunk;

    for(int i = start; i < end; i++) {
        int temp = arr[i];
        arr[i] = arr[N - i - 1];
        arr[N - i - 1] = temp;
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[4];
    int ids[4];

    for(int i = 0; i < N; i++)
        arr[i] = i + 1;

    for(int i = 0; i < num_threads; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, reverse_array, &ids[i]);
    }

    for(int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    for(int i = 0; i < N; i++)
        cout << arr[i] << " ";
}