#include <tudocomp_stat/malloc.hpp>

#include <cstring>

#ifdef __CYGWIN__

#pragma message("malloc overrides are not supported on Cygwin.")

#elif !defined(STATS_DISABLED)

#ifndef __MACH__

constexpr size_t MEMBLOCK_MAGIC = 0xFEDCBA9876543210;

struct block_header_t {
    size_t magic;
    size_t size;
};

inline bool is_managed(block_header_t* block) {
    return (block->magic == MEMBLOCK_MAGIC);
}

extern "C" void* malloc(size_t size) {
    if(!size) return NULL;

    void *ptr = __libc_malloc(size + sizeof(block_header_t));
    if(!ptr) return ptr; // malloc failed

    auto block = (block_header_t*)ptr;
    block->magic = MEMBLOCK_MAGIC;
    block->size = size;

    malloc_callback::on_alloc(size);

    return (char*)ptr + sizeof(block_header_t);
}

extern "C" void free(void* ptr) {
    if(!ptr) return;

    auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if(is_managed(block)) {
        malloc_callback::on_free(block->size);
        __libc_free(block);
    } else {
        __libc_free(ptr);
    }
}

extern "C" void* realloc(void* ptr, size_t size) {
    if(!size) {
        free(ptr);
        return NULL;
    } else if(!ptr) {
        return malloc(size);
    } else {
        auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
        if(is_managed(block)) {
            size_t old_size = block->size;
            void *new_ptr = __libc_realloc(block, size + sizeof(block_header_t));

            auto new_block = (block_header_t*)new_ptr;
            new_block->magic = MEMBLOCK_MAGIC; // just making sure
            new_block->size = size;

            malloc_callback::on_free(old_size);
            malloc_callback::on_alloc(size);

            return (char*)new_ptr + sizeof(block_header_t);
        } else {
            return __libc_realloc(ptr, size);
        }
    }
}

extern "C" void* calloc(size_t num, size_t size) {
    size *= num;
    if(!size) return NULL;

    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

#endif

#endif
