#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define TILE_SIZE 64 

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

uint32_t morton_encode(unsigned int x, unsigned int y) {
    x &= 0x0000ffff;
    y &= 0x0000ffff;
    x = (x | (x << 8)) & 0x00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    y = (y | (y << 8)) & 0x00FF00FF;
    y = (y | (y << 4)) & 0x0F0F0F0F;
    y = (y | (y << 2)) & 0x33333333;
    y = (y | (y << 1)) & 0x55555555;
    return x | (y << 1);
}

typedef struct {
    int start;
    int end;
    int N;
    double *A, *B, *C;
    int s_row, s_col;
} ThreadData;

void* worker_row_major(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start; i < data->end; i++) {
        for (int j = 0; j < data->N; j++) {
            int idx = i * data->N + j;
            data->C[idx] = data->A[idx] + data->B[idx];
        }
    }
    return NULL;
}

void add_row_major_pthread(double *A, double *B, double *C, int N, int num_threads) {
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int chunk = N / num_threads;
    
    for (int t = 0; t < num_threads; t++) {
        thread_data[t].start = t * chunk;
        thread_data[t].end = (t == num_threads - 1) ? N : (t + 1) * chunk;
        thread_data[t].N = N;
        thread_data[t].A = A; thread_data[t].B = B; thread_data[t].C = C;
        pthread_create(&threads[t], NULL, worker_row_major, &thread_data[t]);
    }
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);
}

void* worker_col_major(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int j = data->start; j < data->end; j++) {
        for (int i = 0; i < data->N; i++) {
            int idx = i * data->N + j;
            data->C[idx] = data->A[idx] + data->B[idx];
        }
    }
    return NULL;
}

void add_col_major_pthread(double *A, double *B, double *C, int N, int num_threads) {
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int chunk = N / num_threads;

    for (int t = 0; t < num_threads; t++) {
        thread_data[t].start = t * chunk;
        thread_data[t].end = (t == num_threads - 1) ? N : (t + 1) * chunk;
        thread_data[t].N = N;
        thread_data[t].A = A; thread_data[t].B = B; thread_data[t].C = C;
        pthread_create(&threads[t], NULL, worker_col_major, &thread_data[t]);
    }
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);
}

void* worker_numpy(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start; i < data->end; i++) {
        for (int j = 0; j < data->N; j++) {
            int idx = (i * data->s_row) + (j * data->s_col);
            data->C[idx] = data->A[idx] + data->B[idx];
        }
    }
    return NULL;
}

void add_numpy_pthread(double *A, double *B, double *C, int N, int s_row, int s_col, int num_threads) {
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int chunk = N / num_threads;

    for (int t = 0; t < num_threads; t++) {
        thread_data[t].start = t * chunk;
        thread_data[t].end = (t == num_threads - 1) ? N : (t + 1) * chunk;
        thread_data[t].N = N;
        thread_data[t].A = A; thread_data[t].B = B; thread_data[t].C = C;
        thread_data[t].s_row = s_row; thread_data[t].s_col = s_col;
        pthread_create(&threads[t], NULL, worker_numpy, &thread_data[t]);
    }
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);
}

void* worker_morton(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->start; i < data->end; i++) {
        for (int j = 0; j < data->N; j++) {
            uint32_t idx = morton_encode(i, j);
            if (idx < (uint32_t)(data->N * data->N)) {
                data->C[idx] = data->A[idx] + data->B[idx];
            }
        }
    }
    return NULL;
}

void add_morton_pthread(double *A, double *B, double *C, int N, int num_threads) {
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    int chunk = N / num_threads;

    for (int t = 0; t < num_threads; t++) {
        thread_data[t].start = t * chunk;
        thread_data[t].end = (t == num_threads - 1) ? N : (t + 1) * chunk;
        thread_data[t].N = N;
        thread_data[t].A = A; thread_data[t].B = B; thread_data[t].C = C;
        pthread_create(&threads[t], NULL, worker_morton, &thread_data[t]);
    }
    for (int t = 0; t < num_threads; t++) pthread_join(threads[t], NULL);
}

void* worker_tiled(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int ii = data->start; ii < data->end; ii += TILE_SIZE) {
        for (int jj = 0; jj < data->N; jj += TILE_SIZE) {
            for (int i = ii; i < ii + TILE_SIZE && i < data->N; i++) {
                for (int j = jj; j < jj + TILE_SIZE && j < data->N; j++) {
                    int idx = i * data->N + j;
                    data->C[idx] = data->A[idx] + data->B[idx];
                }
            }
        }
    }
    return NULL;
}

void add_tiled_pthread(double *A, double *B, double *C, int N, int num_threads) {
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    
    int num_blocks = (N + TILE_SIZE - 1) / TILE_SIZE;
    int blocks_per_thread = num_blocks / num_threads;
    if (blocks_per_thread == 0) blocks_per_thread = 1;

    int current_block = 0;
    for (int t = 0; t < num_threads; t++) {
        int start_block = current_block;
        int end_block = start_block + blocks_per_thread;
        if (t == num_threads - 1) end_block = num_blocks;
        
        thread_data[t].start = start_block * TILE_SIZE;
        thread_data[t].end = end_block * TILE_SIZE;
        if (thread_data[t].end > N) thread_data[t].end = N;

        thread_data[t].N = N;
        thread_data[t].A = A; thread_data[t].B = B; thread_data[t].C = C;
        
        if (thread_data[t].start < N) {
             pthread_create(&threads[t], NULL, worker_tiled, &thread_data[t]);
        } else {
             threads[t] = 0; 
        }
        current_block = end_block;
    }
    
    for (int t = 0; t < num_threads; t++) {
        if (threads[t] != 0) pthread_join(threads[t], NULL);
    }
}

int main() {
    int sizes[] = {256, 512, 1024, 2048}; 
    int num_sizes = 4;
    int max_threads = 200; 

    FILE *fp = fopen("results_full_scaling.csv", "w");
    if (!fp) { perror("File open failed"); return 1; }
    
    fprintf(fp, "Size,Method,Threads,Time_Sec\n");
    printf("Starting Pthread Benchmark (1 to %d threads)...\n", max_threads);

    for (int s = 0; s < num_sizes; s++) {
        int N = sizes[s];
        printf("Processing Size: %dx%d\n", N, N);

        double *A = (double*)malloc(N * N * sizeof(double));
        double *B = (double*)malloc(N * N * sizeof(double));
        double *C = (double*)malloc(N * N * sizeof(double));
        
        for(int i=0; i<N*N; i++) { A[i] = 1.0; B[i] = 2.0; }

        for (int th = 1; th <= max_threads; th++) {
            double start, end;
            if (th % 50 == 0) printf("  ... Thread %d\n", th);

            start = get_time();
            add_row_major_pthread(A, B, C, N, th);
            end = get_time();
            fprintf(fp, "%d,RowMajor,%d,%f\n", N, th, end - start);

            start = get_time();
            add_col_major_pthread(A, B, C, N, th);
            end = get_time();
            fprintf(fp, "%d,ColMajor,%d,%f\n", N, th, end - start);

            start = get_time();
            add_numpy_pthread(A, B, C, N, N, 1, th);
            end = get_time();
            fprintf(fp, "%d,NumpyStrided,%d,%f\n", N, th, end - start);

            start = get_time();
            add_morton_pthread(A, B, C, N, th);
            end = get_time();
            fprintf(fp, "%d,Morton,%d,%f\n", N, th, end - start);

            start = get_time();
            add_tiled_pthread(A, B, C, N, th);
            end = get_time();
            fprintf(fp, "%d,Tiled,%d,%f\n", N, th, end - start);
        }

        free(A); free(B); free(C);
    }

    fclose(fp);
    printf("Benchmark Complete. Data saved to results_full_scaling.csv\n");
    return 0;
}
