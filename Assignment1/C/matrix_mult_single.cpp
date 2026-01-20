#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <functional>
#include <string>

using namespace std;
using namespace std::chrono;

class Matrix
{
private:
    int size;
    double **data;

public:
    // Constructor
    Matrix(int n) : size(n)
    {
        data = new double *[n];
        for (int i = 0; i < n; i++)
        {
            data[i] = new double[n];
            fill(data[i], data[i] + n, 0.0);
        }
    }

    // Destructor
    ~Matrix()
    {
        for (int i = 0; i < size; i++)
        {
            delete[] data[i];
        }
        delete[] data;
    }

    // Copy constructor
    Matrix(const Matrix &other) : size(other.size)
    {
        data = new double *[size];
        for (int i = 0; i < size; i++)
        {
            data[i] = new double[size];
            copy(other.data[i], other.data[i] + size, data[i]);
        }
    }

    // Initialize with random values
    void initialize()
    {
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                data[i][j] = static_cast<double>(rand()) / RAND_MAX * 10.0;
            }
        }
    }

    // Clear matrix (set all elements to 0)
    void clear()
    {
        for (int i = 0; i < size; i++)
        {
            fill(data[i], data[i] + size, 0.0);
        }
    }

    // Access operators
    double &operator()(int i, int j) { return data[i][j]; }
    const double &operator()(int i, int j) const { return data[i][j]; }

    // Get size
    int getSize() const { return size; }

    // Get row pointer for SIMD operations
    double *getRow(int i) { return data[i]; }
    const double *getRow(int i) const { return data[i]; }

    void print(int limit = 5) const
    {
        int n = min(size, limit);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                cout << fixed << setprecision(2) << data[i][j] << " ";
            }
            if (n < size)
                cout << "...";
            cout << endl;
        }
        if (n < size)
            cout << "..." << endl;
    }

    // Verify equality with another matrix
    bool equals(const Matrix &other, double tolerance = 1e-6) const
    {
        if (size != other.size)
            return false;
        for (int i = 0; i < size; i++)
        {
            for (int j = 0; j < size; j++)
            {
                if (fabs(data[i][j] - other.data[i][j]) > tolerance)
                {
                    return false;
                }
            }
        }
        return true;
    }
};

// Naive triple loop - Poor cache locality for B matrix
void pattern1_ijk(const Matrix &A, const Matrix &B, Matrix &C)
{
    int n = A.getSize();
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
            {
                sum += A(i, k) * B(k, j);
            }
            C(i, j) = sum;
        }
    }
}

// Reordered loops to access B matrix row-wise
void pattern2_ikj(const Matrix &A, const Matrix &B, Matrix &C)
{
    int n = A.getSize();
    C.clear();
    for (int i = 0; i < n; i++)
    {
        for (int k = 0; k < n; k++)
        {
            double aik = A(i, k);
            for (int j = 0; j < n; j++)
            {
                C(i, j) += aik * B(k, j);
            }
        }
    }
}

// Good for architectures with strided memory access
void pattern3_jik(const Matrix &A, const Matrix &B, Matrix &C)
{
    int n = A.getSize();
    C.clear();
    for (int j = 0; j < n; j++)
    {
        for (int i = 0; i < n; i++)
        {
            double sum = 0.0;
            for (int k = 0; k < n; k++)
            {
                sum += A(i, k) * B(k, j);
            }
            C(i, j) = sum;
        }
    }
}

// Cache-aware blocking with parameterized block size
void pattern4_blocked(const Matrix &A, const Matrix &B, Matrix &C, int blockSize = 64)
{
    int n = A.getSize();
    C.clear();

    // Ensure blockSize doesn't exceed matrix size
    blockSize = min(blockSize, n);

    for (int ii = 0; ii < n; ii += blockSize)
    {
        for (int jj = 0; jj < n; jj += blockSize)
        {
            for (int kk = 0; kk < n; kk += blockSize)
            {
                // Calculate block boundaries
                int i_end = min(ii + blockSize, n);
                int j_end = min(jj + blockSize, n);
                int k_end = min(kk + blockSize, n);

                // Process the block
                for (int i = ii; i < i_end; i++)
                {
                    for (int k = kk; k < k_end; k++)
                    {
                        double aik = A(i, k);
                        for (int j = jj; j < j_end; j++)
                        {
                            C(i, j) += aik * B(k, j);
                        }
                    }
                }
            }
        }
    }
}

// Manual loop unrolling for better instruction-level parallelism
void pattern5_simd(const Matrix &A, const Matrix &B, Matrix &C)
{
    int n = A.getSize();
    C.clear();

    const int UNROLL_FACTOR = 8; // Unroll factor for better pipelining

    for (int i = 0; i < n; i++)
    {
        for (int k = 0; k < n; k++)
        {
            double aik = A(i, k);
            int j = 0;

            // Main unrolled loop
            for (; j <= n - UNROLL_FACTOR; j += UNROLL_FACTOR)
            {
                C(i, j) += aik * B(k, j);
                C(i, j + 1) += aik * B(k, j + 1);
                C(i, j + 2) += aik * B(k, j + 2);
                C(i, j + 3) += aik * B(k, j + 3);
                C(i, j + 4) += aik * B(k, j + 4);
                C(i, j + 5) += aik * B(k, j + 5);
                C(i, j + 6) += aik * B(k, j + 6);
                C(i, j + 7) += aik * B(k, j + 7);
            }

            // Handle remaining elements
            for (; j < n; j++)
            {
                C(i, j) += aik * B(k, j);
            }
        }
    }
}

// Optimized for register reuse and reduced memory traffic
void pattern6_register_blocking(const Matrix &A, const Matrix &B, Matrix &C)
{
    int n = A.getSize();
    C.clear();

    const int UNROLL_I = 2; // Unroll in i dimension
    const int UNROLL_J = 4; // Unroll in j dimension

    for (int i = 0; i < n; i += UNROLL_I)
    {
        for (int j = 0; j < n; j += UNROLL_J)
        {
            // Initialize accumulators
            double acc00 = 0.0, acc01 = 0.0, acc02 = 0.0, acc03 = 0.0;
            double acc10 = 0.0, acc11 = 0.0, acc12 = 0.0, acc13 = 0.0;

            int i_end = min(i + UNROLL_I, n);
            int j_end = min(j + UNROLL_J, n);

            // Compute block
            for (int k = 0; k < n; k++)
            {
                // Prefetch elements
                double a0k = (i < n) ? A(i, k) : 0.0;
                double a1k = (i + 1 < n) ? A(i + 1, k) : 0.0;
                double b0 = B(k, j);
                double b1 = (j + 1 < n) ? B(k, j + 1) : 0.0;
                double b2 = (j + 2 < n) ? B(k, j + 2) : 0.0;
                double b3 = (j + 3 < n) ? B(k, j + 3) : 0.0;

                // Accumulate
                if (i < n)
                {
                    acc00 += a0k * b0;
                    acc01 += a0k * b1;
                    acc02 += a0k * b2;
                    acc03 += a0k * b3;
                }
                if (i + 1 < n)
                {
                    acc10 += a1k * b0;
                    acc11 += a1k * b1;
                    acc12 += a1k * b2;
                    acc13 += a1k * b3;
                }
            }

            // Store results
            if (i < n)
            {
                C(i, j) = acc00;
                if (j + 1 < n)
                    C(i, j + 1) = acc01;
                if (j + 2 < n)
                    C(i, j + 2) = acc02;
                if (j + 3 < n)
                    C(i, j + 3) = acc03;
            }
            if (i + 1 < n)
            {
                C(i + 1, j) = acc10;
                if (j + 1 < n)
                    C(i + 1, j + 1) = acc11;
                if (j + 2 < n)
                    C(i + 1, j + 2) = acc12;
                if (j + 3 < n)
                    C(i + 1, j + 3) = acc13;
            }
        }
    }
}

double measure_time(function<void(const Matrix &, const Matrix &, Matrix &)> pattern_func,
                    const Matrix &A, const Matrix &B, Matrix &C, int runs = 3)
{
    double min_time = numeric_limits<double>::max();

    for (int run = 0; run < runs; run++)
    {
        C.clear();
        auto start = high_resolution_clock::now();
        pattern_func(A, B, C);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);
        double time_seconds = duration.count() / 1e6;
        min_time = min(min_time, time_seconds);
    }

    return min_time;
}

// Measure blocked pattern with specific block size
double measure_time_blocked(function<void(const Matrix &, const Matrix &, Matrix &, int)> pattern_func,
                            const Matrix &A, const Matrix &B, Matrix &C, int blockSize, int runs = 3)
{
    double min_time = numeric_limits<double>::max();

    for (int run = 0; run < runs; run++)
    {
        C.clear();
        auto start = high_resolution_clock::now();
        pattern_func(A, B, C, blockSize);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);
        double time_seconds = duration.count() / 1e6;
        min_time = min(min_time, time_seconds);
    }

    return min_time;
}

int main()
{
    srand(42);

    vector<int> dimensions = {256, 512, 1024, 2048};
    vector<int> blockSizes = {16, 32, 64, 128, 256};

    vector<vector<double>> timingResults(6, vector<double>(dimensions.size(), 0.0));

    cout << "=================================================================" << endl;
    cout << "      SINGLE-THREADED MATRIX MULTIPLICATION BENCHMARK" << endl;
    cout << "=================================================================" << endl;
    cout << "Patterns:" << endl;
    cout << "  1. Standard ijk (Baseline)" << endl;
    cout << "  2. ikj (Better cache locality)" << endl;
    cout << "  3. jik (Column-wise)" << endl;
    cout << "  4. Blocked/Tiled (Cache-aware)" << endl;
    cout << "  5. SIMD Optimized with Loop Unrolling" << endl;
    cout << "  6. Register Blocking" << endl;
    cout << "=================================================================" << endl;

    // Test each matrix dimension
    for (int dim_idx = 0; dim_idx < dimensions.size(); dim_idx++)
    {
        int n = dimensions[dim_idx];

        cout << "\n\n===============================================" << endl;
        cout << "  Testing Matrix Size: " << n << "x" << n << endl;
        cout << "===============================================" << endl;

        Matrix A(n), B(n), C_ref(n), C_test(n);
        A.initialize();
        B.initialize();

        cout << "\nInitializing matrices..." << endl;
        cout << "Memory usage per matrix: "
             << fixed << setprecision(2)
             << (n * n * sizeof(double)) / (1024.0 * 1024.0) << " MB" << endl;

        // Pattern 1: Standard ijk (Baseline)
        cout << "\n--- Pattern 1: Standard ijk (Baseline) ---" << endl;
        C_ref.clear();
        timingResults[0][dim_idx] = measure_time(pattern1_ijk, A, B, C_ref, 2);
        cout << "Time: " << fixed << setprecision(4) << timingResults[0][dim_idx] << " seconds" << endl;

        // Pattern 2: ikj
        cout << "\n--- Pattern 2: ikj (Cache-optimized) ---" << endl;
        timingResults[1][dim_idx] = measure_time(pattern2_ikj, A, B, C_test, 2);
        cout << "Time: " << timingResults[1][dim_idx] << " seconds" << endl;
        if (C_test.equals(C_ref))
        {
            cout << "✓ Results match reference" << endl;
        }
        else
        {
            cout << "✗ Results DO NOT match reference!" << endl;
        }

        // Pattern 3: jik
        cout << "\n--- Pattern 3: jik (Column-wise) ---" << endl;
        timingResults[2][dim_idx] = measure_time(pattern3_jik, A, B, C_test, 2);
        cout << "Time: " << timingResults[2][dim_idx] << " seconds" << endl;
        if (C_test.equals(C_ref))
        {
            cout << "✓ Results match reference" << endl;
        }
        else
        {
            cout << "✗ Results DO NOT match reference!" << endl;
        }

        // Pattern 4: Blocked/Tiled (Test different block sizes)
        cout << "\n--- Pattern 4: Blocked/Tiled Multiplication ---" << endl;
        double best_time_blocked = numeric_limits<double>::max();
        int best_block_size = blockSizes[0];

        for (int blockSize : blockSizes)
        {
            if (blockSize <= n)
            {
                double time = measure_time_blocked(pattern4_blocked, A, B, C_test, blockSize, 2);
                cout << "  Block size " << setw(3) << blockSize << ": "
                     << fixed << setprecision(4) << time << " seconds";

                if (C_test.equals(C_ref))
                {
                    cout << " ✓" << endl;
                    if (time < best_time_blocked)
                    {
                        best_time_blocked = time;
                        best_block_size = blockSize;
                    }
                }
                else
                {
                    cout << " ✗ (Verification failed)" << endl;
                }
            }
        }
        timingResults[3][dim_idx] = best_time_blocked;
        cout << "  → Optimal block size: " << best_block_size
             << " (Time: " << best_time_blocked << "s)" << endl;

        // Pattern 5: SIMD Optimized
        cout << "\n--- Pattern 5: SIMD Optimized ---" << endl;
        timingResults[4][dim_idx] = measure_time(pattern5_simd, A, B, C_test, 2);
        cout << "Time: " << timingResults[4][dim_idx] << " seconds" << endl;
        if (C_test.equals(C_ref))
        {
            cout << "✓ Results match reference" << endl;
        }
        else
        {
            cout << "✗ Results DO NOT match reference!" << endl;
        }

        // Pattern 6: Register Blocking
        cout << "\n--- Pattern 6: Register Blocking ---" << endl;
        timingResults[5][dim_idx] = measure_time(pattern6_register_blocking, A, B, C_test, 2);
        cout << "Time: " << timingResults[5][dim_idx] << " seconds" << endl;
        if (C_test.equals(C_ref))
        {
            cout << "✓ Results match reference" << endl;
        }
        else
        {
            cout << "✗ Results DO NOT match reference!" << endl;
        }

        cout << "\n--- Performance Summary (n=" << n << ") ---" << endl;
        cout << "Pattern 1 (ijk - Baseline):      " << timingResults[0][dim_idx] << "s" << endl;
        cout << "Pattern 2 (ikj):                 " << timingResults[1][dim_idx] << "s  ("
             << timingResults[0][dim_idx] / timingResults[1][dim_idx] << "x speedup)" << endl;
        cout << "Pattern 3 (jik):                 " << timingResults[2][dim_idx] << "s  ("
             << timingResults[0][dim_idx] / timingResults[2][dim_idx] << "x speedup)" << endl;
        cout << "Pattern 4 (Blocked - best):      " << timingResults[3][dim_idx] << "s  ("
             << timingResults[0][dim_idx] / timingResults[3][dim_idx] << "x speedup)" << endl;
        cout << "Pattern 5 (SIMD):                " << timingResults[4][dim_idx] << "s  ("
             << timingResults[0][dim_idx] / timingResults[4][dim_idx] << "x speedup)" << endl;
        cout << "Pattern 6 (Register Blocking):   " << timingResults[5][dim_idx] << "s  ("
             << timingResults[0][dim_idx] / timingResults[5][dim_idx] << "x speedup)" << endl;
    }

    // Generate CSV file for plotting
    ofstream csv_file("matrix_mult_single_thread_results.csv");
    csv_file << "MatrixSize,Pattern1_ijk,Pattern2_ikj,Pattern3_jik,"
             << "Pattern4_Blocked,Pattern5_SIMD,Pattern6_RegBlock" << endl;

    for (int i = 0; i < dimensions.size(); i++)
    {
        csv_file << dimensions[i];
        for (int j = 0; j < 6; j++)
        {
            csv_file << "," << fixed << setprecision(6) << timingResults[j][i];
        }
        csv_file << endl;
    }
    csv_file.close();

    cout << "\n\n=================================================================" << endl;
    cout << "RESULTS SUMMARY" << endl;
    cout << "=================================================================" << endl;
    cout << setw(10) << "Size"
         << setw(12) << "Pattern1"
         << setw(12) << "Pattern2"
         << setw(12) << "Pattern3"
         << setw(12) << "Pattern4"
         << setw(12) << "Pattern5"
         << setw(12) << "Pattern6" << endl;
    cout << "-----------------------------------------------------------------" << endl;

    for (int i = 0; i < dimensions.size(); i++)
    {
        cout << setw(10) << dimensions[i] << "x" << dimensions[i];
        for (int j = 0; j < 6; j++)
        {
            cout << setw(12) << fixed << setprecision(4) << timingResults[j][i];
        }
        cout << endl;
    }

    cout << "\nResults saved to 'matrix_mult_single_thread_results.csv'" << endl;
    cout << "Use the Python script to generate performance plots." << endl;
    cout << "=================================================================" << endl;

    return 0;
}