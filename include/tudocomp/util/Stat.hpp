#ifndef _INCLUDED_STAT_HPP_
#define _INCLUDED_STAT_HPP_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <map>
#include <stdio.h>

namespace tudocomp {
namespace stat {

namespace internal {

    #define MEMBLOCK_SIG 0xFEDCBA9876543210

    struct block_header_t {
        size_t sig;
        size_t id;
        size_t size;
    };
    const size_t block_header_size = sizeof(block_header_t);

    size_t next_id = 0;
    size_t alloc_current = 0;
    size_t alloc_peak = 0;
}

}
}

extern "C" void* __libc_malloc(size_t);
extern "C" void __libc_free(void*);
extern "C" void* __libc_realloc(void*, size_t);

extern void* malloc(size_t size) {
    using namespace tudocomp::stat::internal;

    if(!size) return NULL;

    void *ptr = __libc_malloc(size + block_header_size);

    auto block = (block_header_t*)ptr;
    block->sig = MEMBLOCK_SIG;
    block->id = next_id++;
    block->size = size;

    //fprintf(stderr, "alloc #%zu (%zu bytes)\n", block->id, block->size);

    alloc_current += size;
    if(alloc_current > alloc_peak) { 
        alloc_peak = alloc_current;
    }

    return (char*)ptr + block_header_size;
}

extern void free(void* ptr) {
    using namespace tudocomp::stat::internal;

    if(!ptr) return;

    auto block = (block_header_t*)((char*)ptr - block_header_size);
    if(block->sig == MEMBLOCK_SIG) {
        //fprintf(stderr, "free #%zu (%zu bytes)\n", block->id, block->size);
        alloc_current -= block->size;
        __libc_free(block);
    } else {
        //fprintf(stderr, "free unmanaged block (%p)\n", ptr);
        __libc_free(ptr);
    }
}

extern void* realloc(void* ptr, size_t size) {
    using namespace tudocomp::stat::internal;

    //fprintf(stderr, "realloc(%p, %zu)\n", ptr, size);

    auto block = (block_header_t*)((char*)ptr - block_header_size);
    if(block->sig == MEMBLOCK_SIG) {
        //allocate
        void* ptr2 = malloc(size);

        //copy
        memcpy(ptr2, ptr, block->size);

        //free
        alloc_current -= block->size;
        __libc_free(block);

        return ptr2;
    } else {
        return __libc_realloc(ptr, size);
    }
}

extern void* calloc(size_t num, size_t size) {
    //fprintf(stderr, "calloc(%zu, %zu)\n", num, size);
    
    size *= num;
    if(!size) return NULL;

    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

#endif

