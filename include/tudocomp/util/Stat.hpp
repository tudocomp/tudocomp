#ifndef _INCLUDED_STAT_HPP_
#define _INCLUDED_STAT_HPP_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <map>
#include <time.h>
#include <stdio.h>

namespace tudocomp {
namespace stat {

    ulong current_time_millis() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    struct phase_t {
        ulong time_start;
        size_t mem_current;

        //results
        ulong time_delta;
        size_t mem_peak;
    };

    namespace internal {

        #define MEMBLOCK_SIG 0xFEDCBA9876543210

        struct block_header_t {
            size_t sig;
            size_t id;
            size_t size;
        };

        size_t next_id = 0;
        size_t alloc_current = 0;
        size_t alloc_peak = 0;

        phase_t current_phase;
        bool in_phase = false;
    }

    void begin_phase() {
        using namespace internal;

        in_phase = true;

        current_phase.time_delta = 0;
        current_phase.mem_peak = 0;

        current_phase.mem_current = 0;
        current_phase.time_start = current_time_millis();
    }

    phase_t end_phase() {
        using namespace internal;

        ulong time_end = current_time_millis();
        current_phase.time_delta = time_end - current_phase.time_start;

        in_phase = false;

        return current_phase;
    }
}

}

extern "C" void* __libc_malloc(size_t);
extern "C" void __libc_free(void*);
extern "C" void* __libc_realloc(void*, size_t);

extern void* malloc(size_t size) {
    using namespace tudocomp::stat::internal;

    if(!size) return NULL;

    void *ptr = __libc_malloc(size + sizeof(block_header_t));

    auto block = (block_header_t*)ptr;
    block->sig = MEMBLOCK_SIG;
    block->id = next_id++;
    block->size = size;

    //if(in_phase) {fprintf(stderr, "malloc #%zu (%zu bytes)\n", block->id, block->size);fflush(stderr);}

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

extern void free(void* ptr) {
    using namespace tudocomp::stat::internal;

    if(!ptr) return;

    auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if(block->sig == MEMBLOCK_SIG) {
        //if(in_phase) {fprintf(stderr, "free #%zu (%zu bytes)\n", block->id, block->size); fflush(stderr);}
        alloc_current -= block->size;
        current_phase.mem_current -= block->size;
        __libc_free(block);
    } else {
        if(in_phase) fprintf(stderr, "free unmanaged block (%p)\n", ptr);
        __libc_free(ptr);
    }
}

extern void* realloc(void* ptr, size_t size) {
    using namespace tudocomp::stat::internal;

    if(!size) {
        //if(in_phase) {fprintf(stderr, "realloc %p to 0 (free)\n", ptr); fflush(stderr);}
        free(ptr);
        return NULL;
    } else if(!ptr) {
        //if(in_phase) {fprintf(stderr, "realloc NULL to %zu (malloc)\n", size); fflush(stderr);}
        return malloc(size);
    } else {
        auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
        if(block->sig == MEMBLOCK_SIG) {
            //if(in_phase) {fprintf(stderr, "realloc #%zu from %zu to %zu\n", block->id, block->size, size); fflush(stderr);}

            //allocate
            void* ptr2 = malloc(size);

            //copy
            memcpy(ptr2, ptr, std::min(size, block->size));

            //free
            free(ptr);

            return ptr2;
        } else {
            //if(in_phase) {fprintf(stderr, "realloc unmanaged block (%p) to %zu\n", ptr, size); fflush(stderr);}
            return __libc_realloc(ptr, size);
        }
    }
}

extern void* calloc(size_t num, size_t size) {
    using namespace tudocomp::stat::internal;

    //if(in_phase) {fprintf(stderr, "calloc(%zu, %zu)\n", num, size); fflush(stderr);}
    
    size *= num;
    if(!size) return NULL;

    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

#endif

