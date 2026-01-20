#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// --- Access Patterns ---

// 1. Row-Major (Standard)
void add_row_major(double *a, double *b, double *c, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i * n + j;
            c[idx] = a[idx] + b[idx];
        }
    }
}

// 2. Column-Major (Cache Thrashing)
void add_col_major(double *a, double *b, double *c, int n) {
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            int idx = i * n + j;
            c[idx] = a[idx] + b[idx];
        }
    }
}

// 3. 1D Flattened Loop
void add_1d_flat(double *a, double *b, double *c, int n) {
    int total = n * n;
    for (int k = 0; k < total; k++) {
        c[k] = a[k] + b[k];
    }
}

// 4. Pointer Arithmetic
void add_pointer(double *a, double *b, double *c, int n) {
    int total = n * n;
    double *pa = a, *pb = b, *pc = c;
    for (int k = 0; k < total; k++) {
        *pc++ = *pa++ + *pb++;
    }
}

// 5. Blocked / Tiled Access
void add_blocked(double *a, double *b, double *c, int n) {
    int BLOCK = 64; 
    for (int ii = 0; ii < n; ii += BLOCK) {
        for (int jj = 0; jj < n; jj += BLOCK) {
            for (int i = ii; i < ii + BLOCK && i < n; i++) {
                for (int j = jj; j < jj + BLOCK && j < n; j++) {
                    int idx = i * n + j;
                    c[idx] = a[idx] + b[idx];
                }
            }
        }
    }
}

int main() {
    int sizes[] = {256, 512, 1024, 2048};
    int num_sizes = 4;

    // Open CSV file
    FILE *fp = fopen("results.csv", "w");
    if (fp == NULL) {
        printf("Error opening file for writing!\n");
        return 1;
    }

    // Write CSV Header
    fprintf(fp, "Size,RowMajor,ColMajor,1DFlat,Pointer,Blocked\n");

    // Print Console Table Header
    printf("\nMatrix Addition Benchmark\n");
    printf("+------------+--------------+--------------+--------------+--------------+--------------+\n");
    printf("| %-10s | %-12s | %-12s | %-12s | %-12s | %-12s |\n", 
           "Size (NxN)", "RowMaj (s)", "ColMaj (s)", "1D Flat (s)", "Pointer (s)", "Blocked (s)");
    printf("+------------+--------------+--------------+--------------+--------------+--------------+\n");

    for (int s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        size_t bytes = n * n * sizeof(double);
        
        double *a = (double*)malloc(bytes);
        double *b = (double*)malloc(bytes);
        double *c = (double*)malloc(bytes);

        // Initialize arrays
        for(int k=0; k<n*n; k++) { a[k] = 1.0; b[k] = 2.0; }

        clock_t start, end;
        double t_row, t_col, t_1d, t_ptr, t_blk;

        // Run Benchmarks
        start = clock(); add_row_major(a, b, c, n); end = clock();
        t_row = ((double)(end - start)) / CLOCKS_PER_SEC;

        start = clock(); add_col_major(a, b, c, n); end = clock();
        t_col = ((double)(end - start)) / CLOCKS_PER_SEC;

        start = clock(); add_1d_flat(a, b, c, n); end = clock();
        t_1d = ((double)(end - start)) / CLOCKS_PER_SEC;

        start = clock(); add_pointer(a, b, c, n); end = clock();
        t_ptr = ((double)(end - start)) / CLOCKS_PER_SEC;

        start = clock(); add_blocked(a, b, c, n); end = clock();
        t_blk = ((double)(end - start)) / CLOCKS_PER_SEC;

        // 1. Print formatted row to Console
        printf("| %-10d | %12.6f | %12.6f | %12.6f | %12.6f | %12.6f |\n", 
               n, t_row, t_col, t_1d, t_ptr, t_blk);

        // 2. Write raw data to CSV
        fprintf(fp, "%d,%.6f,%.6f,%.6f,%.6f,%.6f\n", n, t_row, t_col, t_1d, t_ptr, t_blk);

        free(a); free(b); free(c);
    }

    printf("+------------+--------------+--------------+--------------+--------------+--------------+\n");
    printf("Data saved to 'results.csv'. \n\n");

    fclose(fp);
    return 0;
}