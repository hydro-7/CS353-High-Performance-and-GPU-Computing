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

---

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

---

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

---

### 4. Final Conclusion

This experiment demonstrates that C provides no builtin memory safety. Errors like buffer overflows and use after free may silently pass, leading to instability. Using compiler flags such as **`-fsanitize=address`** is a critical best practice for identifying memory corruption and preventing undefined behavior during the development cycle of low level systems.

---




<!-- Lab 2 -->



## Lab 2 : ?
