#include <stdio.h>
#include "umem.h"

// 64 * 1024
int main() {
    if (umeminit(1, BEST_FIT) == 0) {
        printf("Success!\n");
    } else {
        fprintf(stderr, "Error: Failed to initialize memory allocator\n");
        return 1;
    }

    umemstats();

    void *ptr1 = umalloc(7);
    printf("Allocated 100 bytes at %p\n", ptr1);

    void *ptr2 = umalloc(1);
    printf("Allocated 200 bytes at %p\n", ptr2);
    
    umemstats();
    return 0;
}
