#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>

typedef struct {
    int *arr;
    int start;
    int end;
    int local_min;
    int local_max;
} ThreadData;

// Thread function for parallel processing
void* find_min_max_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int l_min = INT_MAX;
    int l_max = INT_MIN;

    for (int i = data->start; i < data->end; i++) {
        if (data->arr[i] < l_min) l_min = data->arr[i];
        if (data->arr[i] > l_max) l_max = data->arr[i];
    }
    data->local_min = l_min;
    data->local_max = l_max;
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void run_comparison(int N) {
    int *arr = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) arr[i] = rand() % 100000000;

    // --- SEQUENTIAL EXECUTION ---
    int seq_min = INT_MAX, seq_max = INT_MIN;
    double start_seq = get_time();
    for (int i = 0; i < N; i++) {
        if (arr[i] < seq_min) seq_min = arr[i];
        if (arr[i] > seq_max) seq_max = arr[i];
    }
    double end_seq = get_time();
    double time_seq = end_seq - start_seq;

    // --- MULTITHREADED EXECUTION ---
    int num_threads = 8;
    pthread_t threads[num_threads];
    ThreadData t_data[num_threads];
    int chunk = N / num_threads;

    double start_thread = get_time();
    for (int i = 0; i < num_threads; i++) {
        t_data[i].arr = arr;
        t_data[i].start = i * chunk;
        t_data[i].end = (i == num_threads - 1) ? N : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, find_min_max_thread, &t_data[i]);
    }

    int thread_min = INT_MAX, thread_max = INT_MIN;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        if (t_data[i].local_min < thread_min) thread_min = t_data[i].local_min;
        if (t_data[i].local_max > thread_max) thread_max = t_data[i].local_max;
    }
    double end_thread = get_time();
    double time_thread = end_thread - start_thread;

    // Output formatted for a table
    printf("%-5d | %-5d | %-5d | %-5d | %-5d | %-10.7f | %-10.7f\n", 
           N, thread_min, thread_max, seq_min, seq_max, time_thread, time_seq);

    free(arr);
}

int main() {
    int sizes1[] = {512, 1024, 2048, 4096, 8192};
    int num_sizes = sizeof(sizes1) / sizeof(sizes1[0]);
    
    printf("For smaller N values, sequential performs better :\n\n");
    printf("%-5s | %-5s | %-5s | %-5s | %-5s | %-10s | %-10s\n", 
           "N", "T-Min", "T-Max", "S-Min", "S-Max", "Time(Thr)", "Time(Seq)");
    printf("----------------------------------------------------------------------------\n");

    for (int i = 0; i < num_sizes; i++) {
        run_comparison(sizes1[i]);
    }

    // -- 
    printf("\n\n");

    int sizes2[] = {5120000, 10240000, 20480000, 40960000, 81920000};
    num_sizes = sizeof(sizes2) / sizeof(sizes2[0]);
    
    printf("For larger N values, threading performs better :\n\n");
    printf("%-5s | %-5s | %-5s | %-5s | %-5s | %-10s | %-10s\n", 
           "N", "T-Min", "T-Max", "S-Min", "S-Max", "Time(Thr)", "Time(Seq)");
    printf("----------------------------------------------------------------------------\n");

    for (int i = 0; i < num_sizes; i++) {
        run_comparison(sizes2[i]);
    }

    return 0;
}