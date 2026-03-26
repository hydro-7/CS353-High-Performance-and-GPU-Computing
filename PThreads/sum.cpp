#include <iostream>
#include <pthread.h>
using namespace std;

#define N 100

int arr[N];
int num_threads = 4;
int partial_sum[4];

void* sum(void* arg) {
    int thread_id = *(int*)arg;
    int chunk = N / num_threads;

    int start = thread_id * chunk;
    int end = start + chunk;

    partial_sum[thread_id] = 0;

    for(int i = start; i < end; i++) {
        partial_sum[thread_id] += arr[i];
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[4];
    int thread_ids[4];

    for(int i = 0; i < N; i++)
        arr[i] = i + 1;

    for(int i = 0; i < num_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, sum, &thread_ids[i]);
    }

    for(int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    int total = 0;
    for(int i = 0; i < num_threads; i++)
        total += partial_sum[i];

    cout << "Sum = " << total << endl;
}