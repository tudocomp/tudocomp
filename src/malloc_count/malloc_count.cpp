#include <malloc_count/malloc_count.hpp>

namespace malloc_count {
    size_t alloc_current = 0;
    size_t alloc_peak = 0;

    size_t next_phase_id = 0;
    phase_t current_phase { SIZE_MAX, 0, 0, 0, 0 };

    void begin_phase() {
        current_phase.id = next_phase_id++;

        current_phase.mem_peak = 0;

        current_phase.mem_current = 0;
        current_phase.time_start = current_time_millis();
    }

    phase_t end_phase() {
        ulong time_end = current_time_millis();
        current_phase.time_delta = time_end - current_phase.time_start;
        current_phase.id = SIZE_MAX; //invalidate

        return current_phase;
    }
}

void* malloc(size_t size) {
    using namespace malloc_count;

    if(!size) return NULL;

    void *ptr = __libc_malloc(size + sizeof(block_header_t));

    auto block = (block_header_t*)ptr;
    block->magic = MEMBLOCK_MAGIC;
    block->phase_id = current_phase.id;
    block->size = size;

    alloc_current += size;
    if(alloc_current > alloc_peak) { 
        alloc_peak = alloc_current;
    }

    current_phase.mem_current += size;
    if(current_phase.mem_current > current_phase.mem_peak) {
        current_phase.mem_peak = current_phase.mem_current;
    }

    return (char*)ptr + sizeof(block_header_t);
}

void free(void* ptr) {
    using namespace malloc_count;

    if(!ptr) return;

    auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if(block->magic == MEMBLOCK_MAGIC) {
        alloc_current -= block->size;

        if(block->phase_id == current_phase.id) {
            current_phase.mem_current -= block->size;
        }

        __libc_free(block);
    } else {
        __libc_free(ptr);
    }
}

void* realloc(void* ptr, size_t size) throw() {
    using namespace malloc_count;

    if(!size) {
        free(ptr);
        return NULL;
    } else if(!ptr) {
        return malloc(size);
    } else {
        auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
        if(block->magic == MEMBLOCK_MAGIC) {
            void* ptr2 = malloc(size);
            memcpy(ptr2, ptr, std::min(size, block->size));
            free(ptr);
            return ptr2;
        } else {
            return __libc_realloc(ptr, size);
        }
    }
}

void* calloc(size_t num, size_t size) {
    size *= num;
    if(!size) return NULL;

    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

