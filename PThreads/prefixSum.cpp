#include <iostream>
#include <pthread.h>
using namespace std;

#define N 8

int arr[N] = {1,2,3,4,5,6,7,8};
int prefix[N];

void* compute_prefix(void* arg) {
    int id = *(int*)arg;

    prefix[id] = 0;
    for(int i = 0; i <= id; i++) {
        prefix[id] += arr[i];
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[N];
    int ids[N];

    for(int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, compute_prefix, &ids[i]);
    }

    for(int i = 0; i < N; i++)
        pthread_join(threads[i], NULL);

    for(int i = 0; i < N; i++)
        cout << prefix[i] << " ";
}