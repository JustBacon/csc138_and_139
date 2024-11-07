#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "umem.h"

static node_t *free_list = NULL;
static header_t *allocated_region = NULL;
static int allocation_algorithm;
static size_t total_allocated = 0;
static int total_allocations = 0;
static int total_deallocations = 0;
static node_t *next_fit_pointer = NULL;

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error: Invalid memory size\n");
        return -1;
    }

    if (free_list != NULL) {
        fprintf(stderr, "Error: Memory Allocater already exist\n");
        return -1;
    }

    int pageSize = getpagesize();
    sizeOfRegion = (sizeOfRegion + pageSize - 1) & ~(pageSize - 1);
    printf("%li\n", sizeOfRegion);

    void *region = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (region == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed");
        return -1;
    }

    allocation_algorithm = allocationAlgo;
    
    free_list = region;
    free_list->size = sizeOfRegion - sizeof(header_t);
    free_list->next = NULL;

    return 0;
}

void *umalloc(size_t size) {
    if (!free_list) return NULL;

    size = (size + (8 - 1)) & ~(8 - 1);
    size_t totalSize = size + sizeof(header_t);

    node_t *prev = NULL;
    node_t *best = NULL;
    node_t *best_prev = NULL;
    node_t *curr = free_list;

    switch (allocation_algorithm) {
        case BEST_FIT:
            while (curr) {
                if (curr->size >= size && (!best || curr->size < best->size)) {
                    best = curr;
                    best_prev = prev;
                }
                prev = curr;
                curr = curr->next;
            }
            break;
        
        case WORST_FIT:
            while (curr) {
                if (curr->size >= size && (!best || curr->size > best->size)) {
                    best = curr;
                    best_prev = prev;
                }
                prev = curr;
                curr = curr->next;
            }
            break;

        case FIRST_FIT:
            while (curr) {
                if (curr->size >= totalSize) {
                    best = curr;
                    best_prev = prev;
                }
                prev = curr;
                curr = curr->next;
            }
            break;
        
        case NEXT_FIT:
            if(next_fit_pointer == NULL) {
                next_fit_pointer = free_list;
            }
            curr = next_fit_pointer;
            while (curr) {
                if (curr->size >= totalSize) {
                    best = curr;
                    best_prev = prev;
                    next_fit_pointer = curr->next;
                    break;
                }
                prev = curr;
                curr = curr->next;
            }
            if (!best) {
                curr = free_list;
                prev = NULL;
                while (curr != next_fit_pointer) {
                    if (curr->size >= totalSize) {
                        best = curr;
                        best_prev = prev;
                        next_fit_pointer = curr->next;
                        break;
                    }
                    prev = curr;
                    curr = curr->next;
                }
            }
            break;
    }

    if (!best) {
        return NULL;
    }

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

    printf("%li\n", header->size);
    total_deallocations++;
    total_allocated -= header->size;

    node_t *new_free = (node_t *)header;
    new_free->size = header->size;
    new_free->next = free_list;
    free_list = new_free;

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
    if (header->size >= size) return ptr;

    void *new_ptr = umalloc(size);
    if (!new_ptr) return NULL;

    ufree(ptr);
    return new_ptr;
}

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
                small_free_memory += current->size;
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
