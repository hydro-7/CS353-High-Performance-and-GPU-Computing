#include <iostream>
#include <pthread.h>
using namespace std;

#define N 100

int arr[N];
int num_threads = 4;
int key = 50;
int found = 0;

void* search(void* arg) {
    int id = *(int*)arg;
    int chunk = N / num_threads;

    int start = id * chunk;
    int end = start + chunk;

    for(int i = start; i < end; i++) {
        if(arr[i] == key) {
            found = 1;
        }
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[4];
    int ids[4];

    for(int i = 0; i < N; i++)
        arr[i] = i;

    for(int i = 0; i < num_threads; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, search, &ids[i]);
    }

    for(int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    if(found)
        cout << "Element Found\n";
    else
        cout << "Not Found\n";
}