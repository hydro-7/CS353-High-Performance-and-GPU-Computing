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

void* find_min_max_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int l_min = INT_MAX, l_max = INT_MIN;
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

// Function to run a single experiment with T threads for all N sizes
double run_bulk_experiment(int num_threads, int* sizes, int num_sizes, FILE* file, int quiet) {
    double total_time_for_config = 0;
    
    if(!quiet) printf("\n--- Results for %d Threads ---\n", num_threads);
    if(!quiet) printf("%-10s | %-10s | %-10s | %-12s\n", "N", "Min", "Max", "Time (s)");

    for (int i = 0; i < num_sizes; i++) {
        int N = sizes[i];
        int *arr = malloc(N * sizeof(int));
        for (int j = 0; j < N; j++) arr[j] = rand() % 1000000;

        pthread_t threads[num_threads];
        ThreadData t_data[num_threads];
        int chunk = N / num_threads;

        double start = get_time();
        for (int t = 0; t < num_threads; t++) {
            t_data[t].arr = arr;
            t_data[t].start = t * chunk;
            t_data[t].end = (t == num_threads - 1) ? N : (t + 1) * chunk;
            pthread_create(&threads[t], NULL, find_min_max_thread, &t_data[t]);
        }

        int g_min = INT_MAX, g_max = INT_MIN;
        for (int t = 0; t < num_threads; t++) {
            pthread_join(threads[t], NULL);
            if (t_data[t].local_min < g_min) g_min = t_data[t].local_min;
            if (t_data[t].local_max > g_max) g_max = t_data[t].local_max;
        }
        double end = get_time();
        double duration = end - start;
        total_time_for_config += duration;

        // Log to file
        fprintf(file, "Threads: %d, N: %d, Min: %d, Max: %d, Time: %.7f\n", 
                num_threads, N, g_min, g_max, duration);
        
        if(!quiet) printf("%-10d | %-10d | %-10d | %-12.7f\n", N, g_min, g_max, duration);

        free(arr);
    }
    return total_time_for_config;
}

int main() {
    int sizes[] = {5120000, 10240000, 20480000, 40960000, 81920000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    FILE *file = fopen("log.txt", "w");
    if (!file) { printf("Error opening file!\n"); return 1; }

    int best_thread_count = 1;
    double best_total_time = 1e18; // Start with a huge number

    printf("Starting experiments. Logging to results.txt...\n");

    // Loop through thread counts: 1 (Sequential), 5, 10, 15 ... 100
    for (int t_count = 1; t_count <= 100; t_count = (t_count == 1) ? 5 : t_count + 5) {
        printf("Testing %d threads...\n", t_count);
        
        // Pass '1' for quiet to avoid cluttering the terminal during the search
        double current_total_time = run_bulk_experiment(t_count, sizes, num_sizes, file, 1);
        
        if (current_total_time < best_total_time) {
            best_total_time = current_total_time;
            best_thread_count = t_count;
        }
    }

    fclose(file);

    // --- Final Output ---
    printf("\n\n==========================================================\n");
    printf("EXPERIMENT COMPLETE. Logs saved to results.txt\n");
    printf("The best performing configuration was: %d Threads\n", best_thread_count);
    printf("==========================================================\n");

    run_bulk_experiment(best_thread_count, sizes, num_sizes, stdout, 0);

    return 0;
}