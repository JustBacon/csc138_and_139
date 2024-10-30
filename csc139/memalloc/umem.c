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

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error: Invalid memory size");
        return -1;
    }

    if (free_list != NULL) {
        fprintf(stderr, "Error: Memory Allocater already exist");
        return -1;
    }

    int pageSize = getpagesize();
    sizeOfRegion = (sizeOfRegion + pageSize - 1) & ~(pageSize - 1);

    void *region = mmap(NULL, sizeOfRegion, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);

    if (region == MAP_FAILED) {
        fprintf(stderr, "Error: mmap failed");
        return -1;
    }
    
    free_list = (node_t*)region;
    free_list->size = sizeOfRegion - sizeof(header_t);
    free_list->next = NULL;

    allocation_algorithm = allocationAlgo;

    return 0;
}

// void *umalloc(size_t size) {
//     if (!allocated_region) return NULL;

//     size = ALIGN(size);
//     node_t *prev = NULL, *curr = free_list, *best = NULL, *best_prev = NULL;

//     while (curr != NULL) {
//         if (curr->size >= size) {
//             if (allocation_algorithm == FIRST_FIT) {
//                 best = curr;
//                 break;
//             }
//             if (allocation_algorithm == BEST_FIT && (!best || curr->size < best->size)) {
//                 best = curr;
//                 best_prev = prev;
//             }
//             if (allocation_algorithm == WORST_FIT && (!best || curr->size > best->size)) {
//                 best = curr;
//                 best_prev = prev;
//             }
//         }
//         prev = curr;
//         curr = curr->next;
//     }

//     if (!best) return NULL;

//     header_t *allocated = (header_t*)((char*)best + sizeof(node_t));
//     allocated->size = size;
//     allocated->magic = MAGIC;

//     if (best->size > size + sizeof(header_t)) {
//         node_t *new_free = (node_t*)((char*)allocated + size + sizeof(header_t));
//         new_free->size = best->size - size - sizeof(header_t);
//         new_free->next = best->next;

//         if (best_prev) best_prev->next = new_free;
//         else free_list = new_free;
//     } else {
//         if (best_prev) best_prev->next = best->next;
//         else free_list = best->next;
//     }

//     total_allocated += size;
//     total_allocations++;
//     return (void*)((char*)allocated + sizeof(header_t));
// }

// int ufree(void *ptr) {
//     if (!ptr) return 0;

//     header_t *block = (header_t*)((char*)ptr - sizeof(header_t));
//     if (block->magic != MAGIC) {
//         fprintf(stderr, "Error: Memory corruption detected at block %p\n", ptr);
//         exit(1);
//     }

//     node_t *free_block = (node_t*)block;
//     free_block->size = block->size;
//     free_block->next = free_list;
//     free_list = free_block;

//     total_deallocations++;
//     return 0;
// }

// void *urealloc(void *ptr, size_t size) {
//     if (!ptr) return umalloc(size);
//     if (size == 0) {
//         ufree(ptr);
//         return NULL;
//     }

//     header_t *block = (header_t*)((char*)ptr - sizeof(header_t));
//     if (block->magic != MAGIC) {
//         fprintf(stderr, "Error: Memory corruption detected at block %p\n", ptr);
//         exit(1);
//     }

//     if (block->size >= size) return ptr;

//     void *new_ptr = umalloc(size);
//     if (!new_ptr) return NULL;

//     memcpy(new_ptr, ptr, block->size);
//     ufree(ptr);
//     return new_ptr;
// }

// void umemstats(void) {
//     size_t free_memory = 0;
//     node_t *current = free_list;
//     while (current) {
//         free_memory += current->size;
//         current = current->next;
//     }

//     double fragmentation = 0.0;
//     printumemstats(total_allocations, total_deallocations, total_allocated, free_memory, fragmentation);
// }
