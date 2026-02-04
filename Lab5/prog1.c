#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <omp.h>
#include <time.h>

#define SIZE 500000000

int main() {
    int *arr = (int*)malloc(SIZE * sizeof(int));
    if (arr == NULL) return 1;

    srand(time(NULL));
    for (int i = 0; i < SIZE; i++) {
        arr[i] = rand();
    }

    long long serial_sum = 0;
    int serial_min = INT_MAX;
    int serial_max = INT_MIN;

    double start_serial = omp_get_wtime();

    for (int i = 0; i < SIZE; i++) {
        int val = arr[i];
        serial_sum += val;
        if (val < serial_min) serial_min = val;
        if (val > serial_max) serial_max = val;
    }

    double end_serial = omp_get_wtime();
    double time_serial = end_serial - start_serial;

    long long par_sum = 0;
    int par_min = INT_MAX;
    int par_max = INT_MIN;

    double start_par = omp_get_wtime();

    #pragma omp parallel for reduction(+:par_sum) reduction(min:par_min) reduction(max:par_max)
    for (int i = 0; i < SIZE; i++) {
        int val = arr[i];
        par_sum += val;
        if (val < par_min) par_min = val;
        if (val > par_max) par_max = val;
    }

    double end_par = omp_get_wtime();
    double time_par = end_par - start_par;

    printf("\nPerformance Comparison (Array Size: %d)\n", SIZE);
    printf("===================================================\n");
    printf("| %-20s | %-20s |\n", "Execution Type", "Time (seconds)");
    printf("|----------------------|----------------------|\n");
    printf("| %-20s | %-20f |\n", "Serial (Single Core)", time_serial);
    printf("| %-20s | %-20f |\n", "Parallel (OpenMP)", time_par);
    printf("===================================================\n");
    
    printf("\nVerification:\n");
    printf("Serial Sum: %lld | Parallel Sum: %lld\n", serial_sum, par_sum);
    printf("Serial Min: %d | Parallel Min: %d\n", serial_min, par_min);
    printf("Serial Max: %d | Parallel Max: %d\n", serial_max, par_max);

    free(arr);
    return 0;
}

