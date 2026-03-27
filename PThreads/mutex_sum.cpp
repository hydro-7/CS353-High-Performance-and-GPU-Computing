#include <iostream>
#include <pthread.h>
using namespace std;

#define N 100

int arr[N];
int num_threads = 4;
int total_sum = 0;

pthread_mutex_t lock;

void* sum(void* arg) {
    int id = *(int*)arg;
    int chunk = N / num_threads;

    int start = id * chunk;
    int end = start + chunk;

    int local_sum = 0;

    for(int i = start; i < end; i++) {
        local_sum += arr[i];
    }

    pthread_mutex_lock(&lock);
    total_sum += local_sum;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[4];
    int ids[4];

    pthread_mutex_init(&lock, NULL);

    for(int i = 0; i < N; i++)
        arr[i] = i + 1;

    for(int i = 0; i < num_threads; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, sum, &ids[i]);
    }

    for(int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    cout << "Total Sum = " << total_sum << endl;

    pthread_mutex_destroy(&lock);
}
