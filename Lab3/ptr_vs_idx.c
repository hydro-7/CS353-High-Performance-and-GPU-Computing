#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define SIZE 500000
#define THREADS 200
#define RUNS 5

int *array;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 0.000001;
}

typedef struct {
    int start;
    int end;
    long long sum;
} Data;



void* sum_using_pointer(void* arg) {
    Data* d = (Data*)arg;
    long long s = 0;

    int *ptr = array + d->start;
    for (int i = d->start; i < d->end; i++) {
        s += *ptr;
        ptr++;
    }

    d->sum = s;
    return NULL;
}



void* sum_using_index(void* arg) {
    Data* d = (Data*)arg;
    long long s = 0;

    for (int i = d->start; i < d->end; i++) {
        s += array[i];
    }

    d->sum = s;
    return NULL;
}


int main() {
    pthread_t threads[THREADS];
    Data data[THREADS];

    array = (int*)malloc(SIZE * sizeof(int));

    for (int i = 0; i < SIZE; i++)
        array[i] = i;

    printf("Expected Sum = 1249975000\n\n");




    printf("Approach 1: Pointer\n");

    for (int r = 1; r <= RUNS; r++) {
        double start = get_time();

        int chunk = SIZE / THREADS;
        for (int i = 0; i < THREADS; i++) {
            data[i].start = i * chunk;
            data[i].end = (i == THREADS - 1) ? SIZE : (i + 1) * chunk;
            data[i].sum = 0;
            pthread_create(&threads[i], NULL, sum_using_pointer, &data[i]);
        }

        long long total = 0;
        for (int i = 0; i < THREADS; i++) {
            pthread_join(threads[i], NULL);
            total += data[i].sum;
        }

        double end = get_time();

        printf("Run %d | Sum = %lld | Time = %.6f sec\n",
               r, total, end - start);
    }




    printf("\nApproach 2: Indexing\n");

    for (int r = 1; r <= RUNS; r++) {
        double start = get_time();

        int chunk = SIZE / THREADS;
        for (int i = 0; i < THREADS; i++) {
            data[i].start = i * chunk;
            data[i].end = (i == THREADS - 1) ? SIZE : (i + 1) * chunk;
            data[i].sum = 0;
            pthread_create(&threads[i], NULL, sum_using_index, &data[i]);
        }

        long long total = 0;
        for (int i = 0; i < THREADS; i++) {
            pthread_join(threads[i], NULL);
            total += data[i].sum;
        }

        double end = get_time();

        printf("Run %d | Sum = %lld | Time = %.6f sec\n",
               r, total, end - start);
    }

    free(array);
    return 0;
}
