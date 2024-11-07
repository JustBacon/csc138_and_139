#include <stdio.h>
#include "umem.h"

void test_allocation();
void test_deallocation();
void test_reallocation();
void test_allocation_strategies();

int main() {
    if (umeminit(64 * 1024, BEST_FIT) != 0) {
        fprintf(stderr, "Error: Failed to initialize memory allocator\n");
        return 1;
    }
    umemstats();
    printf("\n");

    // test_allocation();
    // test_deallocation();
    test_reallocation();

    return 0;
}

void test_allocation() {
    printf("=== TEST ALLOCATION ===\n");

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

    // Memory allocated should be 20304 bytes (does not include the sizeof(header_t))
    umemstats();
}

void test_deallocation() {
    printf("=== TEST DEALLOCATION ===\n");

    void *ptr1 = umalloc(16);
    void *ptr2 = umalloc(32);

    if(ptr1 && ptr2) {
        printf("Allocated 16 bytes at %p and 32 bytes at %p\n", ptr1, ptr2);
        ufree(ptr1);
        printf("Freed memory at %p\n", ptr1);
        ufree(ptr2);
        printf("Freed memory at %p\n", ptr2);
        umemstats();
    }
}

void test_reallocation() {
    printf("=== TEST REALLOCATION ===\n");

    void *ptr1 = umalloc(100);

    if (ptr1) {
        printf("Allocated 100 bytes at %p\n", ptr1);

        void *ptr2 = urealloc(ptr1, 200);
        if (ptr2) {
            printf("Reallocated to 200 bytes at %p\n", ptr2);
        }

        void *ptr3 = urealloc(ptr2, 50);
        if (ptr3) {
            printf("Reallocated to 50 bytes at %p\n", ptr3);
        }
    }
    umemstats();
}