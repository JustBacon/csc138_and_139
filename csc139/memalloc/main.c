#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include "umem.h"

// list of test
void test_allocation();
void test_deallocation();
void test_reallocation();
void test_bestfit();
void test_worstfit();
void test_firstfit();
void test_nextfit();

// used to initialize 64KB
void initialize_memory(int allocation_algorithm);

int main() {
    // array of tests
    void (*tests[])() = {
        test_allocation,
        test_deallocation,
        test_reallocation,
        test_bestfit,
        test_worstfit,
        test_firstfit,
        test_nextfit
    };

    int num_tests = sizeof(tests) / sizeof(tests[0]);
    pid_t pid;

    // using different process to do testing
    for (int i = 0; i < num_tests; i++) {
        if ((pid = fork()) == 0) {
            tests[i]();
            exit(0);
        } else if (pid > 0) {
            wait(NULL);
        } else {
            // Fork failed
            fprintf(stderr, "Error: Fork failed\n");
            return 1;
        }
    }
    
    return 0;
}

// initialize memory allocator
void initialize_memory(int allocation_algorithm) {
    // 64KB
    if (umeminit(64 * 1024, allocation_algorithm) != 0) {
        fprintf(stderr, "Error: Failed to initialize memory allocator\n");
        exit(1);
    }
}

// test umalloc function
void test_allocation() {
    printf("=== TEST ALLOCATION ===\n");
    initialize_memory(BEST_FIT);

    // Actually will allocate 104 bytes because 8-byte aligned chunks of memory
    void *ptr1 = umalloc(100);
    if(ptr1) {
        printf("Allocated 100 bytes at %p\n", ptr1);

    } else {
        printf("Failed allocation of 100 bytes\n");
    }

    void *ptr2 = umalloc(200);
    if(ptr2) {
        printf("Allocated 200 bytes at %p\n", ptr2);
    } else {
        printf("Failed allocation of 200 bytes\n");
    }

    void *ptr3 = umalloc(20000);
    if(ptr3) {
        printf("Allocated 20000 bytes at %p\n", ptr3);
    } else {
        printf("Failed allocation of 20000 bytes\n");
    }

    // Memory allocated should be 104 + 200 + 20000 = 20304 bytes (does not include the sizeof(header_t))
    umemstats();
    printf("\n");
}

// test ufree function
void test_deallocation() {
    printf("=== TEST DEALLOCATION ===\n");
    initialize_memory(BEST_FIT);

    void *ptr1 = umalloc(16);
    void *ptr2 = umalloc(32);

    if(ptr1 && ptr2) {
        printf("Allocated 16 bytes at %p and 32 bytes at %p\n", ptr1, ptr2);
        ufree(ptr1);
        printf("Freed memory at %p\n", ptr1);
        ufree(ptr2);
        printf("Freed memory at %p\n", ptr2);
        umemstats();
        printf("\n");
    }
}

// test urealloc function
void test_reallocation() {
    printf("=== TEST REALLOCATION ===\n");
    initialize_memory(BEST_FIT);

    void *ptr1 = umalloc(100);
    printf("Allocated 100 bytes at %p\n", ptr1);

    // will umalloc(200) since size is bigger
    void *ptr2 = urealloc(ptr1, 200);
    printf("Reallocated to 200 bytes at %p\n", ptr2);
    
    void *ptr3 = urealloc(ptr2, 50);
    printf("Reallocated to 50 bytes at %p\n", ptr3);
    
    printf("ptr1(freed): %p\nptr2(allocated somewhere else): %p\nptr3(same address as ptr2): %p\n", ptr1, ptr2, ptr3);
    
    umemstats();
    printf("\n");
}

// algorithm test for best fit
void test_bestfit() {
    printf("=== TEST BEST FIT ALGORITHM ===\n");
    initialize_memory(BEST_FIT);

    void *ptr1 = umalloc(1000);
    printf("(1)Allocated 1000 bytes at %p\n", ptr1);
    void *ptr2 = umalloc(2000);
    printf("(2)Allocated 2000 bytes at %p\n", ptr2);
    void *ptr3 = umalloc(500);
    printf("(3)Allocated 500 bytes at %p\n", ptr3);
    void *ptr4 = umalloc(1500);
    printf("(4)Allocated 1500 bytes at %p\n", ptr4);

    ufree(ptr2);
    printf("Freed memory at %p\n", ptr2);
    ufree(ptr4);
    printf("Freed memory at %p\n", ptr4);

    // best fit algorithm should choose the 1500-byte for allocating 1200-byte
    void *ptr5 = umalloc(1200);
    printf("Allocated 1200 bytes at %p\n", ptr5);
    printf("BEST FIT should choose (4) 1500-byte for allocating 1200-byte\n");

    umemstats();
    printf("\n");
}

// algorithm test for worst fit
void test_worstfit() {
    printf("=== TEST WORST FIT ALGORITHM ===\n");
    initialize_memory(WORST_FIT);

    void *ptr1 = umalloc(1000);
    printf("(1)Allocated 1000 bytes at %p\n", ptr1);
    void *ptr2 = umalloc(3000);
    printf("(2)Allocated 3000 bytes at %p\n", ptr2);
    void *ptr3 = umalloc(500);
    printf("(3)Allocated 500 bytes at %p\n", ptr3);
    void *ptr4 = umalloc(1500);
    printf("(4)Allocated 1500 bytes at %p\n", ptr4);

    ufree(ptr2);
    printf("Freed memory at %p\n", ptr2);
    ufree(ptr4);
    printf("Freed memory at %p\n", ptr4);

    // worst fit algorithm should choose the 3000-byte for allocating 1200-byte
    void *ptr5 = umalloc(1200);
    printf("Allocated 1200 bytes at %p\n", ptr5);
    printf("WORST FIT should choose the biggest block 60000-byte that has yet to be allocated for allocating 1200-byte.(it will create a block)\n");

    umemstats();
    printf("\n");
}

// algorithm test for first fit
void test_firstfit() {
    printf("=== TEST FIRST FIT ALGORITHM ===\n");
    initialize_memory(FIRST_FIT);

    void *ptr1 = umalloc(1000);
    printf("(1)Allocated 1000 bytes at %p\n", ptr1);
    void *ptr2 = umalloc(3000);
    printf("(2)Allocated 3000 bytes at %p\n", ptr2);
    void *ptr3 = umalloc(500);
    printf("(3)Allocated 500 bytes at %p\n", ptr3);
    void *ptr4 = umalloc(1500);
    printf("(4)Allocated 1500 bytes at %p\n", ptr4);

    ufree(ptr2);
    printf("Freed memory at %p\n", ptr2);
    ufree(ptr4);
    printf("Freed memory at %p\n", ptr4);

    // first fit algorithm should choose the 3000-byte for allocating 1200-byte
    void *ptr5 = umalloc(1200);
    printf("Allocated 1200 bytes at %p\n", ptr5);
    printf("FIRST FIT should choose the first available block. (4) was freed last,\nso it is the first node in the free list. 1500-byte for allocating 1200-byte\n");

    umemstats();
    printf("\n");
}

// algorithm test for next fit
void test_nextfit() {
    printf("=== TEST NEXT FIT ALGORITHM ===\n");
    initialize_memory(NEXT_FIT);

    void *ptr1 = umalloc(1000);
    printf("(1)Allocated 1000 bytes at %p\n", ptr1);
    void *ptr2 = umalloc(2000);
    printf("(2)Allocated 2000 bytes at %p\n", ptr2);
    void *ptr3 = umalloc(1500);
    printf("(3)Allocated 1500 bytes at %p\n", ptr3);
    void *ptr4 = umalloc(500);
    printf("(4)Allocated 500 bytes at %p\n", ptr4);
    void *ptr5 = umalloc(3000);
    printf("(5)Allocated 3000 bytes at %p\n", ptr5);
    void *ptr6 = umalloc(3000);
    printf("(6)Allocated 3000 bytes at %p\n", ptr6);

    ufree(ptr2);
    printf("Freed memory at %p\n", ptr2);
    ufree(ptr4);
    printf("Freed memory at %p\n", ptr4);
    ufree(ptr6);
    printf("Freed memory at %p\n", ptr6);
    // free list right now is ptr6(3000) -> ptr4(500) -> ptr2(2000) -> remaining free

    void *ptr7 = umalloc(1200);
    printf("Allocated 1200 bytes at %p\n", ptr7);
    printf("NEXT FIT should choose (6) 3000-byte for allocating 1200-byte\n");
    void *ptr8 = umalloc(1200);
    printf("Allocated 1200 bytes at %p\n", ptr8);
    printf("NEXT FIT should skip (4) and choose (2) 2000-byte for allocating 1200-byte\n");

    umemstats();
    printf("\n");
}