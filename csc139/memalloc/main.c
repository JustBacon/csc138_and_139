#include <stdio.h>
#include "umem.h"

int main() {
    if (umeminit(1024 * 1024, BEST_FIT) == 0) {
        printf("Success!");
    } else {
        fprintf(stderr, "Error: Failed to initialize memory allocator\n");
        return 1;
    }

    // void *ptr1 = umalloc(100);
    // printf("Allocated 100 bytes at %p\n", ptr1);

    // void *ptr2 = umalloc(200);
    // printf("Allocated 200 bytes at %p\n", ptr2);

    // ufree(ptr1);
    // printf("Freed memory at %p\n", ptr1);

    // umemstats();

    // void *ptr3 = urealloc(ptr2, 300);
    // printf("Reallocated 200 bytes to 300 bytes at %p\n", ptr3);

    // umemstats();

    // ufree(ptr3);
    // printf("Freed memory at %p\n", ptr3);

    // umemstats();
    return 0;
}
