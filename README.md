# CS353-High-Performance-and-GPU-Computing



<!-- Lab 1 -->



## Lab 1 : Memory Safety and Error Detection in C

### 1. Objective

The objective of this lab experiment is to study common memory-related issues in C programs, observe how they behave under normal compilation, and analyze how these issues are detected and reported using compiler sanitization flags.

**Key Focus Areas:**

* Reproducing **Segmentation Faults**.
* Identifying **Undefined Behavior**.
* Analyzing **Heap Memory Errors**.
* Understanding the importance of compiler diagnostics and runtime checks.



### 2. Tools & Compilation Setup

To monitor memory behavior effectively, we utilize the **AddressSanitizer (ASan)** provided by the GCC compiler.

| Flag | Description |
| --- | --- |
| **`-fsanitize=address`** | Enables AddressSanitizer to detect buffer overflows, use-after-free, and invalid memory access. |
| **`-g`** | Includes debugging information for precise line-number error tracing. |

**Compilation Command:**

```bash
gcc -fsanitize=address -g file.c 

```



### 3. Experiments & Observations

#### 3.1 Excessive Memory Allocation

**Code Snippet:**

```c
int *arr = malloc(100000000000000000000 * sizeof(int));

```

* **Without Flags:** The program crashes with a generic segmentation fault.
* **With `-fsanitize=address`:** Provides a clear error: `Requested allocation size exceeds maximum supported size`.
* **Conclusion:** AddressSanitizer explicitly reports the cause of failure, whereas normal compilation provides no meaningful diagnostic.

#### 3.2 Heap Buffer Overflow (Out-of-Bounds Access)

**Code Snippet:**

```c
printf("%d\n", arr[1001]);
printf("%d\n", arr[100000]);

```

| Access Index | Behavior Without Flags | Behavior With Flags |
| --- | --- | --- |
| **`arr[1001]`** | Prints `0` (appears valid/silent error). | **`heap-buffer-overflow`** |
| **`arr[100000]`** | **Segmentation Fault** | **`unknown address`** |

* **Explanation:** Small overflows may access adjacent memory that appears readable, leading to "silent" bugs. ASan places "redzones" around allocations to catch these immediately.

#### 3.3 Use After Free

**Code Snippet:**

```c
free(arr);
printf("%d\n", arr[0]);

```

* **Without Flags:** Results in undefined behavior (prints garbage or previous values).
* **With Flags:** Reports a **`heap-use-after-free`** error.
* **Conclusion:** Using freed memory is unsafe; sanitization prevents these bugs from remaining silent and dangerous.

#### 3.4 NULL Pointer Dereference

**Code Snippet:**

```c
arr = NULL;
printf("%d\n", arr[0]);

```

* **Without Flags:** **Segmentation Fault**.
* **With Flags:** **`unknown address`** error.
* **Conclusion:** Sanitizers improve diagnostic clarity by pinpointing exactly where the invalid memory access occurred.



### 4. Final Conclusion

This experiment demonstrates that C provides no builtin memory safety. Errors like buffer overflows and use after free may silently pass, leading to instability. Using compiler flags such as **`-fsanitize=address`** is a critical best practice for identifying memory corruption and preventing undefined behavior during the development cycle of low level systems.

---




<!-- Lab 2 -->



## Lab 2 : Min / Max via Multithreading 

### 1. Objective

The objective of this lab experiment is to design and implement a highperformance multithreaded C program to compute the minimum and maximum values from a set of N elements, and to evaluate its performance against a sequential approach.

**Key Focus Areas:**

* Understanding **parallel reduction** techniques for min–max computation.
* Analyzing **scalability** of the multithreaded solution as input size **N increases**.
* Comparing **execution time** of multithreaded and sequential implementations.
* Ensuring **correctness and consistency** of results across multiple runs.
* Measuring and observing performance for varying input sizes:
  **N = 512, 1024, 2048, 4096, 8192**.


### 2. Implementation Strategy

To achieve high-performance results, the following strategies were implemented:

* **Domain Decomposition:** The input array is divided into  equal-sized chunks, where each chunk is assigned to a separate POSIX thread (**Pthread**).
* **Local Reduction:** Each thread independently computes the local minimum and maximum of its assigned range to avoid synchronization overhead (like mutexes) within the loop.
* **Global Aggregation:** Once all threads finish execution (`pthread_join`), the main thread performs a final reduction on the local results to determine the global min/max.

### 3. Experiments & Methodology

The lab was divided into two distinct phases of experimentation:

#### **Part 1: Performance Scaling (prog1.c)**

We compared the multithreaded approach (8 threads) against a sequential approach across two distinct ranges of :

1. **Small Scale ( to ):** To observe the impact of **thread creation overhead** and context switching.
2. **Large Scale ( to ):** To observe the **speedup** provided by parallel execution on compute-heavy tasks.

#### **Part 2: Thread Optimization & Logging (prog2.c)**

We automated an experiment to find the "Sweet Spot" for concurrency:

* **Variable Threading:** Tested thread counts from **1 to 100** (increments of 5).
* **Data Logging:** All results (Thread count, , Min, Max, Time) were logged to `log.txt`.
* **Optimal Configuration Search:** The program identifies the thread count that yields the lowest total execution time across all test cases.

### 4. Observations & Results

| Configuration | Small N Observations | Large N Observations |
| --- | --- | --- |
| **Sequential** | **Faster.** Low overhead; the CPU finishes the task before a thread can even be spawned. | **Slower.** Bound by a single core's clock speed. |
| **Multithreaded** | **Slower.** The time taken for `pthread_create` and `pthread_join` exceeds the computation time. | **Faster.** Drastic reduction in execution time as work is distributed across CPU cores. |

#### **Key Findings:**

1. **Consistency:** In all runs, `T-Min == S-Min` and `T-Max == S-Max`, proving the reliability of the parallel reduction logic.
2. **Scalability:** The program demonstrates **Linear Scalability** for large , where doubling the input size results in a predictable change in execution time.
3. **The Over-threading Penalty:** In `prog2.c`, we observed that performance improves as thread count approaches the number of logical CPU cores, but degrades significantly when threads exceed 50+ due to **excessive context switching**.

### 5. Conclusion

This experiment highlights that **parallelism is not a "silver bullet."** For small datasets, the overhead of managing threads outweighs the benefits of parallel processing. However, for high-performance computing tasks involving millions of elements, multithreading is essential for reducing latency. The optimal performance is typically achieved when the number of threads matches the hardware's logical core count.

---













<!-- Lab 3 -->

















## Lab 3 : Parallel Sum Computation Using Threads and Memory Access Analysis

### 1. Objective
The objective of this experiment is to study and analyze the performance impact of different parallel programming and memory access strategies in C. Specifically, the experiment focuses on:
- Dynamically allocating a large array of **5,00,000 elements**
- Creating **200 parallel threads** to compute the sum of these elements
- Comparing multiple synchronization and memory access approaches
- Observing how design choices affect execution time and scalability


### 2. Implementation Strategy

The experiment was implemented using **POSIX Threads (pthreads)** in C.  
Two separate source files were created, each focusing on a different performance aspect:

#### 2.1 `global_vs_local.c`
This file compares two approaches for accumulating partial sums computed by threads.

##### Approach 1: Global Sum with Mutex Locking
Each thread computes a partial sum and updates a shared global variable using a mutex.

```c
pthread_mutex_lock(&lock);
global_sum += local_sum;
pthread_mutex_unlock(&lock);
````

This approach ensures correctness but introduces heavy synchronization overhead.



##### Approach 2: Thread-Local Sum (Reduced Locking)

Each thread maintains its own private sum and avoids locking during computation.

```c
long long thread_sum = 0;
for (int i = start; i < end; i++) {
    thread_sum += arr[i];
}
partial_sums[thread_id] = thread_sum;
```

All partial sums are combined after all threads complete execution.



#### 2.2 `ptr_vs_idx.c`

This file analyzes the impact of different array access methods on performance.

##### Approach 1: Pointer-Based Access

The array is accessed using pointer arithmetic.

```c
sum += *(ptr + i);
```

Although functionally correct, this approach showed slower execution in practice.



##### Approach 2: Direct Index-Based Access

The array is accessed using standard indexing.

```c
sum += arr[i];
```

This method consistently performed better during experiments.



### 3. Experiments & Methodology

#### 3.1 Experimental Setup

* Number of elements: **5,00,000**
* Number of threads: **200**
* Platform: Linux-based system
* Compiler: `gcc` with pthread support
* Timing measured using high-resolution timers

#### 3.2 Methodology

1. Initialize the dynamically allocated array with known values.
2. Divide the array evenly among all threads.
3. Execute each approach independently.
4. Measure total execution time for each method.
5. Compare results based on performance and correctness.

Each test was run multiple times to ensure consistency.



### 4. Observations & Results

#### 4.1 Mutex-Based Global Sum

* Execution time was **significantly higher**.
* Threads frequently blocked while waiting for the mutex.
* High contention caused serialized execution despite multiple threads.

**Observation:**
Mutex locking inside frequently executed code sections severely degrades performance in highly parallel workloads.



#### 4.2 Thread-Local Sum

* Execution time was **much lower**.
* Threads executed independently without synchronization overhead.
* Only a final aggregation step was required.

**Observation:**
Using thread-local variables reduces contention and allows true parallelism, making it the most efficient approach.



#### 4.3 Pointer-Based Array Access (`*(ptr + index)`)

* Observed to be **slower** than index-based access.
* Requires additional address computation at runtime.
* Less friendly to compiler optimizations such as auto-vectorization.

**Reason for Slower Performance:**
Pointer arithmetic involves explicit address calculation for every access, which can inhibit certain compiler optimizations. It also complicates alias analysis, preventing aggressive instruction reordering and cache prefetching.



#### 4.4 Index-Based Array Access (`arr[index]`)

* Observed to be **faster** and more consistent.
* Compiler can better optimize indexed access.
* Improved cache locality and instruction-level parallelism.

**Reason for Faster Performance:**
Array indexing provides clearer memory access patterns to the compiler, enabling optimizations such as loop unrolling, vectorization, and efficient cache utilization.



### 5. Conclusion

From this experiment, the following conclusions were drawn:

* Excessive mutex usage can nullify the benefits of multithreading.
* Thread-local computation with minimal synchronization yields the best performance.
* Memory access patterns significantly impact execution time.
* Direct array indexing is generally faster and more optimization-friendly than pointer arithmetic.

Overall, careful synchronization and memory-access design are essential for high-performance parallel programs.

---





























<!-- Lab 4 -->



## Lab 4: Parallel Sparse Matrix Multiplication using Pthreads

### 1. Objective

To implement and benchmark parallel sparse matrix multiplication using POSIX Threads (Pthreads). The primary goal is to analyze the trade offs between synchronization overhead and load balancing by comparing five different thread scheduling strategies.

### 2. Problem Statement

Sparse matrix multiplication presents a unique challenge for parallelization due to **irregular workloads**. Unlike dense matrices, where every row computation takes the same amount of time, sparse matrix rows vary significantly in the number of non zero elements.

* **Static assignment** often leads to **load imbalance** (some cores idle while others work).
* **Dynamic assignment** fixes imbalance but introduces **critical section overhead** (locking).
* **Goal:** Determine which scheduling approach minimizes execution time and maximizes core utilization.

### 3. Methodology & Approaches

We investigated five specific scheduling strategies to distribute the row-wise multiplication tasks across  threads.

#### **A. Static Scheduling**

1. **Block Scheduling:** The matrix is divided into  continuous blocks. Thread  processes rows .
* *Observation:* Fastest when data is uniform; prone to severe imbalance if non-zeros are clustered.


2. **Cyclic Scheduling:** Rows are distributed in a round-robin fashion (Thread  processes rows ).
* *Observation:* Statistically better load balancing than Block scheduling without the cost of locks, but suffers from poorer cache locality.



#### **B. Dynamic Scheduling**

3. **Fine-Grained:** Threads lock a mutex to grab a **single row** index from a global counter.
* *Observation:* Perfect load balancing but prohibitive synchronization overhead due to frequent locking.


4. **Chunked:** Threads lock a mutex to grab a fixed-size **chunk** (e.g., 64 rows).
* *Observation:* A compromise that reduces lock contention while maintaining good load balance.


5. **Guided:** Threads grab chunks that exponentially decrease in size.
* *Observation:* Optimizes overhead at the start (large chunks) and fairness at the end (small chunks).



### 4. Key Implementation Details

The core logic relies on a shared "row kernel" and varying thread functions.

**The Core Kernel (Row Multiplication):**

```c
// Computes a single row result for C = A * B
void compute_row(int r, CSRMatrix *A, CSRMatrix *B, double *C) {
    for (int j = A->row_ptr[r]; j < A->row_ptr[r+1]; j++) {
        int col_a = A->col_ind[j];
        double val_a = A->values[j];
        // Accumulate partial products
        for (int k = B->row_ptr[col_a]; k < B->row_ptr[col_a+1]; k++) {
            C[r * N + B->col_ind[k]] += val_a * B->values[k];
        }
    }
}

```

**Algorithm for Dynamic Scheduling (Generalized):**

```text
Global: row_counter = 0, Mutex Lock

Thread Function:
    Loop until row_counter >= Total_Rows:
        Lock Mutex
        Get current row_counter
        Increment row_counter by CHUNK_SIZE
        Unlock Mutex
        
        Process rows [current ... current + CHUNK_SIZE]

```

**Running the commands:**

```bash
gcc -O3 -pthread prog.c -o prog
./prog

```

### 5. Performance Analysis & Discussion

#### **Q1: Minimizing Execution Time**

Our benchmarking indicates that execution time follows a "U-curve" relative to the thread count.

* **Optimal Thread Count:** The minimum execution time was consistently achieved when the number of threads matched the number of physical CPU cores (typically 4-8 on standard machines).
* **Over-Subscription:** Increasing threads beyond the physical core count (e.g., 32 threads on an 8-core machine) increased execution time. This is due to **Context Switching overhead**, where the OS wastes cycles switching between active threads rather than performing computation.
* **Memory Bottleneck:** As the thread count rose, the performance gains diminished due to saturation of the memory bandwidth, confirming that sparse matrix multiplication is memory-bound.

#### **Q2: Improving Fair Core Utilization**

Achieving 100% utilization of all cores required addressing the irregular data structure.

* **Why Static Failed:** Static Block scheduling resulted in "tail latency," where one thread working on a dense section of the matrix kept the program running long after other threads had finished and idled.
* **The Optimal Solution:** The **Dynamic Guided** and **Dynamic Chunked** approaches provided the best core utilization. By decoupling row assignment from thread ID, these methods ensured that "light" threads effectively picked up the slack for "heavy" threads. The **Guided** approach specifically excelled by minimizing the "fight for the lock" at the beginning of execution while ensuring granular balancing at the end.

### 6. Results

*Test Environment: Matrix Size , Sparsity .*

| Approach Name | Best Time (s) | Optimal Threads |
| --- | --- | --- |
| **Static (Block)** | *0.0217* | *16* |
| **Dynamic (Fine)** | *0.0203* | *16* |
| **Dynamic (Chunked)** | *0.0202* | *16* |
| **Static (Cyclic)** | *0.0239* | *16* |
| **Dynamic (Guided)** | *0.0205* | *16* |

### 7. Conclusion

This experiment demonstrated that for irregular workloads like sparse matrix operations, Dynamic approaches typically offer the best trade-off between overhead and load balancing. While Static approaches are faster for uniform data, they fail to utilize multi-core architectures efficiently when data density varies.



































<!-- Lab 5 -->







## Lab 5: Parallel Reduction & Sorting Check (OpenMP vs Pthreads)

### 1. Objective
The objective of this lab is to explore parallel programming paradigms using OpenMP and POSIX Threads (Pthreads). Specifically, the lab aims to:
1.  Implement **parallel reduction** to compute the sum, minimum, and maximum of a large array efficiently.
2.  Design and compare three different parallel approaches (OpenMP Shared Flag, OpenMP Reduction, and Manual Pthreads) to check if a large list is sorted.
3.  Analyze the performance trade-offs, execution time, and CPU core utilization of these approaches.

---

### 2. Task 1: Min, Max, and Sum using OpenMP

#### 2.1 Approach & Explanation
In this task, we process a large array (500 million elements) to find the sum, minimum, and maximum values.

* **Concept (Layman):** Imagine counting a huge stack of cash. Instead of one person counting it all, you split the stack among 4 people. Each person counts their pile and finds the smallest/largest bill. At the end, they combine their totals to get the final result.
* **Technical Design:**
    * We use the `#pragma omp parallel for` directive to parallelize the loop.
    * **The Reduction Clause:** The key mechanism is `reduction(+:sum) reduction(min:min_val) reduction(max:max_val)`.
    * OpenMP automatically creates a **private copy** of these variables for each thread to prevent race conditions. Each thread works on its local copy, and at the end of the parallel region, OpenMP combines them using the specified operator (`+`, `min`, `max`).

#### 2.2 Compilation & Execution
**Command used:**
```bash
gcc -O3 -fopenmp prog1.c -o prog1
./prog1

```

#### 2.3 Observations

Based on the execution with 500 million elements:

| Execution Type | Time (seconds) |
| --- | --- |
| **Serial (Single Core)** | 0.284774 |
| **Parallel (OpenMP)** | 0.152710 |

* **Speedup:** The parallel implementation is approximately **1.86x faster** than the serial version.
* **Verification:** The sum, min, and max values matched exactly between serial and parallel executions, confirming correctness.

<img width="636" height="214" alt="image" src="https://github.com/user-attachments/assets/8eda065a-9167-45b2-8242-1e136e553397" />

---

### 3. Task 2: Parallel Sorted Check (Comparison of 3 Approaches)

#### 3.1 Approaches Designed

We implemented three distinct strategies to check if an array is sorted in ascending order.

**Approach 1: OpenMP with Shared Flag (Simulated Early Exit)**

* **Concept:** Like a group of inspectors looking for a defect. If anyone finds one, they shout "Stop!", and everyone stops working.
* **Technical:** Uses a `volatile bool sorted` shared flag. Threads constantly check `if (!sorted) continue;` inside the loop.
* **Trade-off:** Fast for unsorted lists (early exit), but suffers from **cache coherence traffic** (CPUs constantly talking to ensure they have the latest flag value).

**Approach 2: OpenMP with Reduction (Count Violations)**

* **Concept:** Inspectors work in isolation. They don't talk to each other. They just write down how many errors they found. At the very end, we sum up the errors.
* **Technical:** Uses `#pragma omp parallel for reduction(+:violations)`. Threads maintain private counters and merge them at the end.
* **Trade-off:** Very high CPU utilization and throughput for *sorted* lists (no synchronization overhead), but wasteful for unsorted lists as it cannot stop early.

**Approach 3: Manual Pthreads (Data Decomposition)**

* **Concept:** Manually assigning specific chunks of work to specific workers. "Worker A, you take items 0 to 100. Worker B, you take 101 to 200."
* **Technical:** We calculate `start` and `end` indices manually and use `pthread_create` to spawn threads. We handle the "boundary condition" (checking `arr[i] > arr[i+1]` across thread chunks) explicitly.
* **Trade-off:** Provides low-level control but incurs higher **thread creation overhead** compared to OpenMP's optimized thread pool.

#### 3.2 Compilation & Execution

**Command used:**

```bash
g++ -O3 -fopenmp -pthread prog2.cpp -o prog2
./prog2

```


#### 3.3 Observations & Analysis

**Performance Data:**

| List Size | OMP Flag (s) | OMP Reduction (s) | Pthreads (s) |
| --- | --- | --- | --- |
| **10,000** | 0.000007 | 0.000011 | 0.000602 |
| **1,000,000** | 0.000158 | 0.000049 | 0.000658 |
| **10,000,000** | 0.001891 | 0.001036 | 0.001836 |

<img width="1085" height="245" alt="image" src="https://github.com/user-attachments/assets/1faff32c-eeff-42b9-bd0d-bae442445596" />

**1. Execution Time Analysis:**

* **Small Lists:** Manual Pthreads is significantly slower (e.g., 0.0006s vs 0.000007s for size 10k). This is due to the overhead of creating new OS threads, whereas OpenMP reuses threads effectively.
* **Large Lists:** OMP Reduction becomes the fastest approach (0.0010s vs 0.0018s for OMP Flag). The lack of synchronization during the loop allows it to scale better than the Shared Flag approach.

<img width="1200" height="600" alt="image" src="https://github.com/user-attachments/assets/2ad9096f-900a-4cc6-be73-4a81f4a24858" />


**2. CPU Utilization Analysis:**

* **OMP Reduction:** Achieved **100% CPU utilization** consistently across larger sizes. This confirms that cores were fully saturated with work without waiting for locks or flags.
* **Manual Pthreads:** Showed lower utilization (~28% max) in this specific run, likely due to the overhead dominance relative to the actual computation work.

<img width="1200" height="600" alt="image" src="https://github.com/user-attachments/assets/29ba0f74-84e0-4667-ae48-db3f5df59dfc" />

---

### 4. Conclusion

1. **Reduction Efficiency:** For aggregating data (Sum/Min/Max), OpenMP reductions are highly efficient and require minimal code changes compared to serial versions.
2. **Overhead Matters:** For small datasets, the overhead of creating manual threads (Pthreads) far outweighs the parallel speedup. OpenMP handles this much better.
3. **Synchronization vs. Throughput:**
* **Shared Flag** is best when we expect to fail early (unsorted data).
* **Reduction** is best when we expect to process the whole dataset (sorted data) because it eliminates communication overhead during the loop.




















<!-- Lab 6 -->


