#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <string>
#include <omp.h>

using namespace std;

struct CpuSnapshot {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
};

CpuSnapshot get_cpu_snapshot() {
    CpuSnapshot snap = {0};
    ifstream file("/proc/stat");
    if (!file.is_open()) return snap;

    string line, label;
    getline(file, line);
    istringstream iss(line);

    iss >> label >> snap.user >> snap.nice >> snap.system >> snap.idle
        >> snap.iowait >> snap.irq >> snap.softirq >> snap.steal;

    return snap;
}

double calculate_cpu_usage(const CpuSnapshot& start, const CpuSnapshot& end) {
    unsigned long long active_start = start.user + start.nice + start.system + start.irq + start.softirq + start.steal;
    unsigned long long idle_start = start.idle + start.iowait;

    unsigned long long active_end = end.user + end.nice + end.system + end.irq + end.softirq + end.steal;
    unsigned long long idle_end = end.idle + end.iowait;

    unsigned long long active_diff = active_end - active_start;
    unsigned long long idle_diff = idle_end - idle_start;
    unsigned long long total_diff = active_diff + idle_diff;

    if (total_diff == 0) return 0.0;
    return (double)active_diff / total_diff * 100.0;
}

void generate_list(vector<int>& arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = i; 
    }
}

bool omp_check_early_exit(const vector<int>& arr) {
    int n = arr.size();
    volatile bool sorted = true;

    #pragma omp parallel for shared(sorted)
    for (int i = 0; i < n - 1; i++) {
        if (!sorted) continue;
        if (arr[i] > arr[i+1]) {
            sorted = false;
        }
    }
    return sorted;
}

bool omp_check_reduction(const vector<int>& arr) {
    int n = arr.size();
    int violations = 0;

    #pragma omp parallel for reduction(+:violations)
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i+1]) {
            violations++;
        }
    }
    return (violations == 0);
}

// Changed bool* to int* to avoid vector<bool> addressing issues
void thread_worker(const vector<int>* arr, int start, int end, int* result) {
    *result = 1; // 1 = true
    for (int i = start; i < end; i++) {
        if ((*arr)[i] > (*arr)[i+1]) {
            *result = 0; // 0 = false
            break;
        }
    }
}

bool manual_thread_check(const vector<int>& arr) {
    int n = arr.size();
    int num_threads = thread::hardware_concurrency();
    vector<thread> threads;
    
    // Changed vector<bool> to vector<int>
    vector<int> results(num_threads, 1);
    
    int chunk_size = (n - 1) / num_threads;

    for (int i = 0; i < num_threads; i++) {
        int start = i * chunk_size;
        int end = (i == num_threads - 1) ? (n - 1) : (i + 1) * chunk_size;
        
        threads.emplace_back(thread_worker, &arr, start, end, &results[i]);
    }

    bool global_sorted = true;
    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
        if (results[i] == 0) global_sorted = false;
    }
    return global_sorted;
}

int main() {
    vector<int> sizes = {10000, 25000, 50000, 1000000, 10000000};

    ofstream csv_file("prog2_values.csv");
    csv_file << "Size,OMP_Flag_Time,OMP_Flag_CPU,OMP_Red_Time,OMP_Red_CPU,Manual_Time,Manual_CPU\n";

    cout << "Sorted Check Benchmark (Averaged over many iterations)\n";
    cout << string(95, '-') << endl;
    cout << left << setw(10) << "Size"
         << setw(15) << "OMP Flag(s)" << setw(10) << "CPU%"
         << setw(15) << "OMP Red(s)" << setw(10) << "CPU%"
         << setw(15) << "Manual(s)" << setw(10) << "CPU%" << endl;
    cout << string(95, '-') << endl;

    for (int n : sizes) {
        vector<int> arr(n);
        generate_list(arr, n);

        CpuSnapshot s1, s2;
        double start_time, end_time;
        
        // 1. OMP Flag
        s1 = get_cpu_snapshot();
        start_time = omp_get_wtime();

        int current_iterations = 5000;
        
        for(int k=0; k<current_iterations; k++) omp_check_early_exit(arr);
        end_time = omp_get_wtime();
        s2 = get_cpu_snapshot();
        
        double t1 = (end_time - start_time) / current_iterations;
        double c1 = calculate_cpu_usage(s1, s2);

        // 2. OMP Reduction
        s1 = get_cpu_snapshot();
        start_time = omp_get_wtime();
        for(int k=0; k<current_iterations; k++) omp_check_reduction(arr);
        end_time = omp_get_wtime();
        s2 = get_cpu_snapshot();

        double t2 = (end_time - start_time) / current_iterations;
        double c2 = calculate_cpu_usage(s1, s2);

        // 3. Manual Threads
        s1 = get_cpu_snapshot();
        start_time = omp_get_wtime();
        for(int k=0; k<current_iterations; k++) manual_thread_check(arr);
        end_time = omp_get_wtime();
        s2 = get_cpu_snapshot();

        double t3 = (end_time - start_time) / current_iterations;
        double c3 = calculate_cpu_usage(s1, s2);

        cout << left << setw(10) << n
             << fixed << setprecision(6) << setw(15) << t1 << setprecision(1) << setw(10) << c1
             << fixed << setprecision(6) << setw(15) << t2 << setprecision(1) << setw(10) << c2
             << fixed << setprecision(6) << setw(15) << t3 << setprecision(1) << setw(10) << c3 << endl;

        csv_file << n << "," << t1 << "," << c1 << "," << t2 << "," << c2 << "," << t3 << "," << c3 << "\n";
    }

    cout << string(95, '-') << endl;
    csv_file.close();
    return 0;
}