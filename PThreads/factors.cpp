#include <iostream>
#include <pthread.h>
using namespace std;

#define NUM_THREADS 4

int n;

void* find_factors(void* arg) {
    int thread_id = *(int*)arg;

    int start = (n / NUM_THREADS) * thread_id + 1;
    int end = (thread_id == NUM_THREADS - 1) ? n : (n / NUM_THREADS) * (thread_id + 1);

    for (int i = start; i <= end; i++) {
        if (n % i == 0) {
            cout << "Factor: " << i << endl;
        }
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    cout << "Enter number: ";
    cin >> n;

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, find_factors, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
