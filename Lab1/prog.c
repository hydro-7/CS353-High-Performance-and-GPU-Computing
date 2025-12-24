#include<stdio.h>
#include <stdbool.h>
#include <stdlib.h> 

bool isPrime(int num) {
    if(num <= 1) return false;
    for(int i = 2; i * i <= num; i++) {
        if(num % i == 0) return false;
    }
    return true;
}

int main() {

    // Command To Be Used : gcc -fsanitize=address -g file.c
    // Here we use the flags -fsanitize & -g to report the errors in the code
    int n = 1000;
    int *arr = malloc(n * sizeof(int));

    
    // 1. When we try to allocate memory larger than the size of a block
    // It leads to an error -> "Requested allocation size exceeds maximum supported size"
    // Without the use of flags, this just leads to a normal segmentation fault 

    int *arr = malloc(100000000000000000000 * sizeof(int));      

    

    // ============ Prime Number Logic  ============
    int ptr = 0;
    for(int i = 0; i < 100000; i++) {
        if(isPrime(i) == true) arr[ptr++] = i;
        if(ptr == 1000) break;
    }

    // for(int i = 0; i < ptr; i++) printf("%d ", arr[i]);
    // printf("\n");

    // ==============================================



    // 2. When we try to access a memory index that is out of bounds from what has been allocated 
    // Without using the flags during compilation it will still give an output -> 0 
    // This is because the entire block has been brought, and it can be accessed directly 
    // But if the access index is so large that it exceeds the block size, it will give an error even without the flag
    
    printf("%d\n", arr[1001]);               // Without Flags -> returns 0 ; With Flags -> gives "heap-buffer-overflow" error 
    printf("%d\n", arr[100000]);             // Without Flags -> Seg Fault ; With Flags -> gives "unknown address"
    
    // 3. When we try to access the array after freeing the allocation 
    // This in normal C is considered as undefined behaviour 
    // <>
    // If it is free() -> print(arr[0]) : 
    // without flags : undefined behaviour (either grabage / previously declared values) ; with flags : "heap-use-after-free" error
    // <>
    // If it is free() -> arr = NULL -> print(arr[0]) :
    // without flags : Seg Fault ; with flags : "unknown address"

    free(arr);
    arr = NULL;
    printf("%d\n", arr[0]);

    return 0;
}

