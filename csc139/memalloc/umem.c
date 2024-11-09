#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "umem.h"
#include <string.h>

static node_t *free_list = NULL;        // first node of the free list
static int allocation_algorithm;        // determine which algorithm. BEST_FIT, WORST_FIT,etc
static size_t total_allocated = 0;      // used for umemstats. how many bytes allocated
static int total_allocations = 0;       // keep track of umalloc() usage
static int total_deallocations = 0;     // keep track of ufree() usage
static node_t *next_fit_pointer = NULL; // used for NEXT_FIT strategy

// find_block searches for a free block to allocate
node_t *find_block(size_t size, size_t totalSize);

// used for splitting block
node_t *find_best_previous_block(node_t *best);

// algorithm functions
node_t *find_best_fit_block(size_t size);
node_t *find_worst_fit_block(size_t size);
node_t *find_first_fit_block(size_t totalSize);
node_t *find_next_fit_block(size_t totalSize);

// Initializes memory allocator
// sizeOfRegion is the number of bytes to request from OS using mmap()
// allocationAlgo determines which algorithm to use
int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    // check if user uses 0 or negative size
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error: Invalid memory size\n");
        return -1;
    }

    // check if umeminit is called more than once in a process
    if (free_list != NULL) {
        fprintf(stderr, "Error: Memory Allocater already exist\n");
        return -1;
    }

    // round up the requested memory in units of page size
    int pageSize = getpagesize();
    sizeOfRegion = (sizeOfRegion + pageSize - 1) & ~(pageSize - 1);

    // use mmap() to request memory from OS
    void *region = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // check if mmap() failed
    if (region == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed");
        return -1;
    }

    // set allocation algorithm
    allocation_algorithm = allocationAlgo;
    
    // set free list pointer
    free_list = region;
    free_list->size = sizeOfRegion - sizeof(header_t);
    free_list->next = NULL;

    return 0;
}

// takes size in bytes to be allocated and returns a pointer
void *umalloc(size_t size) {
    // if free_list does not exist return NULL
    if (!free_list) return NULL;

    // round up the requested memory in units of 8
    size = (size + (8 - 1)) & ~(8 - 1);
    size_t totalSize = size + sizeof(header_t);

    // find best block to use to allocate memory based on algorithm
    node_t *best = find_block(size, totalSize);

    if (!best) {
        return NULL;
    }

    // use for splitting block
    node_t *best_prev = find_best_previous_block(best);

    // Splitting block
    if (best->size >= totalSize + sizeof(node_t)) {
        node_t *new_free = (node_t *)((char *)best + totalSize);
        new_free->size = best->size - totalSize;
        new_free->next = best->next;

        if (best_prev) {
            best_prev->next = new_free;
        } else {
            free_list = new_free;
        }
    } else {
        if (best_prev) {
            best_prev->next = best->next;
        } else {
            free_list = best->next;
        }
    }

    header_t *header = (header_t *)best;
    header->size = size;
    header->magic = MAGIC;

    total_allocations++;
    total_allocated += size;

    return (void *)((char *)header + sizeof(header_t));
}

// use for splitting block
node_t *find_best_previous_block(node_t *best) {
    node_t *prev = NULL;
    node_t *curr = free_list;

    while (curr && curr != best) {
        prev = curr;
        curr = curr->next;
    }

    return prev;
}

// find best block to use based on allocation algorithm
node_t *find_block(size_t size, size_t totalSize) {
    switch (allocation_algorithm) {
        case BEST_FIT:
            return find_best_fit_block(size);
        case WORST_FIT:
            return find_worst_fit_block(size);
        case FIRST_FIT:
            return find_first_fit_block(totalSize);
        case NEXT_FIT:
            return find_next_fit_block(totalSize);
    }
    return NULL;
}

// algorithm for BEST FIT
node_t *find_best_fit_block(size_t size) {
    node_t *best = NULL;
    node_t *curr = free_list;

    while (curr) {
        if (curr->size >= size && (!best || curr->size < best->size)) {
            best = curr;
        }
        curr = curr->next;
    }
    return best;
}

// algorithm for WORST FIT
node_t *find_worst_fit_block(size_t size) {
    node_t *best = NULL;
    node_t *curr = free_list;

    while (curr) {
        if (curr->size >= size && (!best || curr->size > best->size)) {
            best = curr;
        }
        curr = curr->next;
    }
    return best;
}

// algorithm for FIRST FIT
node_t *find_first_fit_block(size_t totalSize) {
    node_t *curr = free_list;

    while (curr) {
        if (curr->size >= totalSize) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// algorithm for NEXT FIT
node_t *find_next_fit_block(size_t totalSize) {
    if (next_fit_pointer == NULL) {
        next_fit_pointer = free_list;
    }
    node_t *prev = NULL;
    node_t *curr = next_fit_pointer;

    // Search from the current pointer onwards
    while (curr) {
        if (curr->size >= totalSize) {
            next_fit_pointer = curr->next;
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }

    // If no suitable block is found, wrap around and search from the beginning
    curr = free_list;
    while (curr != next_fit_pointer) {
        if (curr->size >= totalSize) {
            next_fit_pointer = curr->next;
            return curr;
        }
        prev = curr;
        curr = curr->next;
    }

    return NULL;
}

// frees memory object that ptr points to
int ufree(void *ptr) {
    if (!ptr) return 0;

    header_t *header = (header_t*)((char*)ptr - sizeof(header_t));
    if (header->magic != MAGIC) {
        fprintf(stderr, "Error: Memory corruption detected at block %p\n", ptr);
        exit(1);
    }
    
    node_t *curr = free_list;
    while (curr != NULL) {
        if ((void *)curr == (void *)header) {
            fprintf(stderr, "Error: Double free detected at block %p\n", ptr);
            exit(1);
        }
        curr = curr->next;
    }

    total_deallocations++;
    total_allocated -= header->size;

    node_t *new_free = (node_t *)header;
    new_free->size = header->size;
    new_free->next = free_list;
    free_list = new_free;

    // check neighbouring blocks to see if they are also free
    // Coalesce adjacent free blocks
    curr = free_list;
    while (curr != NULL && curr->next != NULL) {
        node_t *next = curr->next;
        if((char *)curr + curr->size + sizeof(header_t) == (char *)next) {
            curr->size += next->size + sizeof(header_t);
            curr->next = next->next;
        } else if ((char *)curr - next->size - sizeof(header_t) == (char *)next) {
            next->size += curr->size + sizeof(header_t);
            free_list = next;
            curr = next;
        } else {
            curr = curr->next;
        }
    }

    return 0;
}

void *urealloc(void *ptr, size_t size) {
    // if pointer is null use umalloc()
    if (ptr == NULL){
        return umalloc(size);
    } 
    // if size is 0 use ufree()
    if (size == 0) {
        ufree(ptr);
        return NULL;
    }

    // 8-byte aligned pointers
    size = (size + (8 - 1)) & ~(8 - 1);
    size_t totalSize = size + sizeof(header_t);

    header_t *header = (header_t*)((char*)ptr - sizeof(header_t));

    // check memory corruption using MAGIC number
    if (header->magic != MAGIC) {
        fprintf(stderr, "Error: Memory corruption detected at block %p\n", ptr);
        exit(1);
    }

    // if the block size is sufficient, return the same pointer
    if (header->size >= size) {
        return ptr;
    }

    void *new_ptr = umalloc(size);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, header->size);

    ufree(ptr);
    return new_ptr;
}

// show stats
void umemstats(void) {
    size_t free_memory = 0;
    size_t largest_free_block = 0;
    size_t small_free_memory = 0;

    node_t *current = free_list;
    while (current != NULL) {
        free_memory += current->size;

        if (current->size > largest_free_block) {
            largest_free_block = current->size;
        }

        current = current->next;
    }

    if (largest_free_block > 0) {
        size_t fragmentation_threshold = largest_free_block / 2;
    
        current = free_list;
        while (current != NULL) {
            if (current->size < fragmentation_threshold) {
                small_free_memory += current->size + sizeof(header_t);
            }
            current = current->next;
        }

    }

    double fragmentation = 0.0;
    if (free_memory > 0) {
        fragmentation = ((double)small_free_memory / (double)free_memory) * 100.0;
    }

    printumemstats(total_allocations, total_deallocations, total_allocated, free_memory, fragmentation);
}
