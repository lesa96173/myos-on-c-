#include "../include/common.h"

#define HEAP_START 0x200000
#define HEAP_SIZE  (1024 * 1024) 

struct MemBlock {
    int size;
    bool is_free;
};

void init_memory() {
    char* ram = (char*)HEAP_START;
    for(int i = 0; i < 64; i++) ram[i] = 0; 
    MemBlock* start = (MemBlock*)HEAP_START;
    start->size = HEAP_SIZE - sizeof(MemBlock);
    start->is_free = true;
}

void* malloc(int size) {
    int offset = 0;
    while (offset < HEAP_SIZE) {
        MemBlock* block = (MemBlock*)(HEAP_START + offset);
        if (block->size <= 0) break; 
        if (block->is_free && block->size >= size) {
            if (block->size > size + (int)sizeof(MemBlock)) {
                int old_size = block->size;
                block->size = size;
                block->is_free = false;
                MemBlock* next = (MemBlock*)(HEAP_START + offset + sizeof(MemBlock) + size);
                next->size = old_size - size - sizeof(MemBlock);
                next->is_free = true;
                return (void*)(HEAP_START + offset + sizeof(MemBlock));
            } else {
                block->is_free = false;
                return (void*)(HEAP_START + offset + sizeof(MemBlock));
            }
        }
        offset += sizeof(MemBlock) + block->size;
    }
    return nullptr;
}

void free(void* ptr) {
    if (!ptr) return;
    MemBlock* block = (MemBlock*)((char*)ptr - sizeof(MemBlock));
    block->is_free = true;
}