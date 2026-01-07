#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define SIZE 50000
#define THREADS 200
#define RUNS 5

int *array;

/* ---------- Time Function ---------- */
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 0.000001;
}

/* ---------- APPROACH 1 ---------- */
/* Global sum with mutex */

long long global_sum = 0;
pthread_mutex_t lock;

typedef struct {
    int start;
    int end;
} Data;

void* sum_mutex(void* arg) {
    Data* d = (Data*)arg;
    long long temp = 0;

    for (int i = d->start; i < d->end; i++)
        temp += array[i];

    pthread_mutex_lock(&lock);
    global_sum += temp;
    pthread_mutex_unlock(&lock);

    return NULL;
}

/* ---------- APPROACH 2 ---------- */
/* Thread local sums (no mutex) */

long long partial_sum[THREADS];

void* sum_no_mutex(void* arg) {
    int id = *(int*)arg;
    int chunk = SIZE / THREADS;
    int start = id * chunk;
    int end = (id == THREADS - 1) ? SIZE : start + chunk;

    long long sum = 0;
    for (int i = start; i < end; i++)
        sum += array[i];

    partial_sum[id] = sum;
    return NULL;
}

/* ---------- MAIN ---------- */

int main() {
    pthread_t threads[THREADS];
    Data data[THREADS];
    int ids[THREADS];

    array = (int*)malloc(SIZE * sizeof(int));

    for (int i = 0; i < SIZE; i++)
        array[i] = i;

    printf("Expected Sum = 1249975000\n\n");

    /* ===== APPROACH 1 ===== */
    printf("Approach 1: Global sum with mutex\n");

    for (int r = 1; r <= RUNS; r++) {
        global_sum = 0;
        pthread_mutex_init(&lock, NULL);

        double start = get_time();

        int chunk = SIZE / THREADS;
        for (int i = 0; i < THREADS; i++) {
            data[i].start = i * chunk;
            data[i].end = (i == THREADS - 1) ? SIZE : (i + 1) * chunk;
            pthread_create(&threads[i], NULL, sum_mutex, &data[i]);
        }

        for (int i = 0; i < THREADS; i++)
            pthread_join(threads[i], NULL);

        double end = get_time();

        printf("Run %d | Sum = %lld | Time = %.6f sec\n",
               r, global_sum, end - start);

        pthread_mutex_destroy(&lock);
    }

    /* ===== APPROACH 2 ===== */
    printf("\nApproach 2: Thread local sum (faster)\n");

    for (int r = 1; r <= RUNS; r++) {

        for (int i = 0; i < THREADS; i++)
            partial_sum[i] = 0;

        double start = get_time();

        for (int i = 0; i < THREADS; i++) {
            ids[i] = i;
            pthread_create(&threads[i], NULL, sum_no_mutex, &ids[i]);
        }

        for (int i = 0; i < THREADS; i++)
            pthread_join(threads[i], NULL);

        long long total = 0;
        for (int i = 0; i < THREADS; i++)
            total += partial_sum[i];

        double end = get_time();

        printf("Run %d | Sum = %lld | Time = %.6f sec\n",
               r, total, end - start);
    }

    free(array);
    return 0;
}
