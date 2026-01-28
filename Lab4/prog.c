#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>

#define N 4096           // Matrix Dimension (4096 x 4096)
#define SPARSITY 0.96    // 96% Zeros (Highly Sparse)
#define CHUNK_SIZE 64    // Base Chunk size for Dynamic/Guided methods

// --- Data Structures ---
typedef struct {
    int *row_ptr;   // CSR Row Pointer
    int *col_ind;   // CSR Column Indices
    double *values; // CSR Values
    int nnz;        // Number of Non-Zero elements
    int rows;
    int cols;
} CSRMatrix;

typedef struct {
    int thread_id;
    int num_threads;
    CSRMatrix *A;
    CSRMatrix *B;
    double *C;
} ThreadData;

// --- Global Variables for Synchronization ---
int global_row_counter = 0;
pthread_mutex_t queue_mutex;

// --- Helper Functions ---
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

void reset_result(double *C, int size) {
    for (int i = 0; i < size; i++) C[i] = 0.0;
}

// Convert Dense Matrix to CSR Format
CSRMatrix convertToCSR(double *matrix, int rows, int cols) {
    CSRMatrix csr;
    csr.rows = rows;
    csr.cols = cols;
    csr.row_ptr = (int*)malloc((rows + 1) * sizeof(int));

    // First pass: Count Non-Zeros
    int nnz = 0;
    for (int i = 0; i < rows * cols; i++) if (matrix[i] != 0.0) nnz++;
    csr.nnz = nnz;

    // Allocate memory
    csr.col_ind = (int*)malloc(nnz * sizeof(int));
    csr.values = (double*)malloc(nnz * sizeof(double));

    // Second pass: Fill arrays
    int count = 0;
    for (int i = 0; i < rows; i++) {
        csr.row_ptr[i] = count;
        for (int j = 0; j < cols; j++) {
            double val = matrix[i * cols + j];
            if (val != 0.0) {
                csr.values[count] = val;
                csr.col_ind[count] = j;
                count++;
            }
        }
    }
    csr.row_ptr[rows] = count;
    return csr;
}

// Generate Random Sparse Matrix
void generateRandomSparseMatrix(double *matrix, int rows, int cols, float sparsity) {
    srand(42);
    for (int i = 0; i < rows * cols; i++) {
        if (((double)rand() / RAND_MAX) > sparsity) {
            matrix[i] = ((double)rand() / RAND_MAX) * 10.0;
        } else {
            matrix[i] = 0.0;
        }
    }
}

// Observation Function
int isSparse(double *matrix, int rows, int cols) {
    long long zero_count = 0;
    long long total = (long long)rows * cols;
    for (int i = 0; i < total; i++) {
        if (matrix[i] == 0.0) zero_count++;
    }
    double ratio = (double)zero_count / total;
    printf(">> Observation: Matrix Density is %.2f%% (Sparsity: %.2f%%)\n", 
           (1.0-ratio)*100, ratio * 100);
    return (ratio > 0.8);
}

// --- CORE PROCESSING KERNEL ---
void compute_row(int r, CSRMatrix *A, CSRMatrix *B, double *C) {
    for (int j = A->row_ptr[r]; j < A->row_ptr[r+1]; j++) {
        int col_a = A->col_ind[j];      // Index in A
        double val_a = A->values[j];    // Value in A

        // Multiply A value by corresponding row in B
        for (int k = B->row_ptr[col_a]; k < B->row_ptr[col_a+1]; k++) {
            int col_b = B->col_ind[k];
            double val_b = B->values[k];
            C[r * N + col_b] += val_a * val_b;
        }
    }
}

// --- APPROACH 1: Static Scheduling (Block) ---
void* method_static_block(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int chunk = data->A->rows / data->num_threads;
    int start = data->thread_id * chunk;
    int end = (data->thread_id == data->num_threads - 1) ? data->A->rows : start + chunk;

    for (int i = start; i < end; i++) {
        compute_row(i, data->A, data->B, data->C);
    }
    pthread_exit(NULL);
}

// --- APPROACH 2: Dynamic Scheduling (Fine Grained) ---
void* method_dynamic_fine(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int r;
    while(1) {
        pthread_mutex_lock(&queue_mutex);
        if(global_row_counter >= data->A->rows) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        r = global_row_counter++;
        pthread_mutex_unlock(&queue_mutex);
        
        compute_row(r, data->A, data->B, data->C);
    }
    pthread_exit(NULL);
}

// --- APPROACH 3: Dynamic Scheduling (Chunked) ---
void* method_dynamic_chunk(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int start, end;
    while(1) {
        pthread_mutex_lock(&queue_mutex);
        if(global_row_counter >= data->A->rows) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        start = global_row_counter;
        global_row_counter += CHUNK_SIZE;
        pthread_mutex_unlock(&queue_mutex);

        end = (start + CHUNK_SIZE > data->A->rows) ? data->A->rows : start + CHUNK_SIZE;
        for(int i = start; i < end; i++) {
            compute_row(i, data->A, data->B, data->C);
        }
    }
    pthread_exit(NULL);
}

// --- APPROACH 4: Static Scheduling (Cyclic) ---
// Distributes rows in a round-robin format (0, P, 2P...)
// Good for load balancing if sparsity is uneven, without mutex overhead.
void* method_static_cyclic(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (int i = data->thread_id; i < data->A->rows; i += data->num_threads) {
        compute_row(i, data->A, data->B, data->C);
    }
    pthread_exit(NULL);
}

// --- APPROACH 5: Dynamic Scheduling (Guided) ---
// Chunk size decreases exponentially. Formula: remaining_rows / (2 * num_threads)
// Balances low overhead (start) with good load balancing (end).
void* method_dynamic_guided(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int start, end, chunk;
    int total_rows = data->A->rows;

    while(1) {
        pthread_mutex_lock(&queue_mutex);
        int remaining = total_rows - global_row_counter;
        
        if(remaining <= 0) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        chunk = remaining / (2 * data->num_threads);
        
        if (chunk < CHUNK_SIZE) chunk = CHUNK_SIZE;
        if (chunk > remaining) chunk = remaining;

        start = global_row_counter;
        global_row_counter += chunk;
        pthread_mutex_unlock(&queue_mutex);

        end = start + chunk;
        for(int i = start; i < end; i++) {
            compute_row(i, data->A, data->B, data->C);
        }
    }
    pthread_exit(NULL);
}

int main() {
    printf("=== Sparse Matrix Multiplication Benchmark (Pthreads) ===\n");
    printf("Matrix Size: %d x %d\n", N, N);
    
    double *matA = (double*)calloc(N * N, sizeof(double));
    double *matB = (double*)calloc(N * N, sizeof(double));
    double *matC = (double*)calloc(N * N, sizeof(double));

    printf("Generating Matrices (Target Sparsity: %.0f%%)...\n", SPARSITY * 100);
    generateRandomSparseMatrix(matA, N, N, SPARSITY);
    generateRandomSparseMatrix(matB, N, N, SPARSITY);

    if (isSparse(matA, N, N)) {
        printf(">> Matrix detected as Sparse. Converting to CSR for optimization.\n");
    }

    CSRMatrix csrA = convertToCSR(matA, N, N);
    CSRMatrix csrB = convertToCSR(matB, N, N);

    free(matA); 
    free(matB);

    int thread_counts[] = {1, 2, 4, 8, 16};
    int num_tests = 5;
    int num_methods = 5; 

    struct Result {
        char name[40];
        int optimal_threads;
        double min_time;
    } results[5]; 

    for (int method = 0; method < num_methods; method++) {
        
        if (method == 0) sprintf(results[method].name, "Static (Block)");
        else if (method == 1) sprintf(results[method].name, "Dynamic (Fine)");
        else if (method == 2) sprintf(results[method].name, "Dynamic (Chunked)");
        else if (method == 3) sprintf(results[method].name, "Static (Cyclic)");
        else if (method == 4) sprintf(results[method].name, "Dynamic (Guided)");

        printf("\n--- Testing: %s ---\n", results[method].name);
        
        double best_time = 1e9;
        int best_threads = 0;

        for (int t_idx = 0; t_idx < num_tests; t_idx++) {
            int n_threads = thread_counts[t_idx];

            reset_result(matC, N * N);
            global_row_counter = 0;
            pthread_mutex_init(&queue_mutex, NULL);
            
            pthread_t threads[n_threads];
            ThreadData t_data[n_threads];

            double start = get_time();

            for (int i = 0; i < n_threads; i++) {
                t_data[i].thread_id = i;
                t_data[i].num_threads = n_threads;
                t_data[i].A = &csrA;
                t_data[i].B = &csrB;
                t_data[i].C = matC;

                if (method == 0) pthread_create(&threads[i], NULL, method_static_block, &t_data[i]);
                else if (method == 1) pthread_create(&threads[i], NULL, method_dynamic_fine, &t_data[i]);
                else if (method == 2) pthread_create(&threads[i], NULL, method_dynamic_chunk, &t_data[i]);
                else if (method == 3) pthread_create(&threads[i], NULL, method_static_cyclic, &t_data[i]);
                else if (method == 4) pthread_create(&threads[i], NULL, method_dynamic_guided, &t_data[i]);
            }

            for (int i = 0; i < n_threads; i++) {
                pthread_join(threads[i], NULL);
            }

            double end = get_time();
            double elapsed = end - start;
            pthread_mutex_destroy(&queue_mutex);

            printf("Threads: %2d | Time: %.4f sec\n", n_threads, elapsed);

            if (elapsed < best_time) {
                best_time = elapsed;
                best_threads = n_threads;
            }
        }

        results[method].optimal_threads = best_threads;
        results[method].min_time = best_time;
    }

    printf("\n\n=============================================================\n");
    printf("                 FINAL PERFORMANCE SUMMARY                   \n");
    printf("=============================================================\n");
    printf("| %-25s | %-15s | %-10s |\n", "Approach Name", "Optimal Threads", "Min Time");
    printf("|---------------------------|-----------------|------------|\n");
    for(int i = 0; i < num_methods; i++) {
        printf("| %-25s | %-15d | %-9.4fs |\n", 
               results[i].name, 
               results[i].optimal_threads, 
               results[i].min_time);
    }
    printf("=============================================================\n");

    // Cleanup
    free(csrA.row_ptr); free(csrA.col_ind); free(csrA.values);
    free(csrB.row_ptr); free(csrB.col_ind); free(csrB.values);
    free(matC);

    return 0;
}