#include <malloc_count/malloc_count.hpp>

namespace malloc_count {
    #define PHASE_STACK_SIZE 16

    #define MEMBLOCK_MAGIC 0xFEDCBA9876543210
    struct block_header_t {
        size_t magic;
        size_t size;
    };

    size_t next_phase_id = 1;

    phase_t pstack[PHASE_STACK_SIZE];
    int pcur = -1;

    bool paused = false;

    void begin_phase() {
        pcur++;
        if(pcur >= PHASE_STACK_SIZE) throw "phase stack overflow";

        pstack[pcur].id = next_phase_id++;
        pstack[pcur].mem_off = (pcur > 0) ? pstack[pcur - 1].mem_current : 0;
        pstack[pcur].mem_peak = 0;
        pstack[pcur].mem_current = 0;
        pstack[pcur].time_start = current_time_millis();
    }

    phase_t end_phase() {
        if(pcur < 0) throw "ending a phase that never began";

        pstack[pcur].time_end = current_time_millis();
        pstack[pcur].id = SIZE_MAX; //invalidate

        return pstack[pcur--];
    }

    void pause_phase() {
        paused = true;
    }

    void resume_phase() {
        paused = false;
    }

    inline bool is_managed(block_header_t* block) {
        return (block->magic == MEMBLOCK_MAGIC);
    }

    inline void count_malloc(size_t size) {
        if(!paused && pcur >= 0) {
            for(int i = 0; i <= pcur; i++) {
                pstack[i].mem_current += size;
                if(pstack[i].mem_current > pstack[i].mem_peak) {
                    pstack[i].mem_peak = pstack[i].mem_current;
                }
            }
        }
    }

    inline void count_free(size_t size) {
        if(!paused && pcur >= 0) {
            for(int i = 0; i <= pcur; i++) {
                pstack[i].mem_current -= size;
            }
        }
    }
}

#ifdef __CYGWIN__

#pragma message("malloc overrides are not supported on Cygwin.")

#elif !defined(STATS_DISABLED)

void* malloc(size_t size) {
    using namespace malloc_count;

    if(!size) return NULL;

    void *ptr = __libc_malloc(size + sizeof(block_header_t));

    auto block = (block_header_t*)ptr;
    block->magic = MEMBLOCK_MAGIC;
    block->size = size;

    count_malloc(size);

    return (char*)ptr + sizeof(block_header_t);
}

void free(void* ptr) {
    using namespace malloc_count;

    if(!ptr) return;

    auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if(is_managed(block)) {
        count_free(block->size);
        __libc_free(block);
    } else {
        __libc_free(ptr);
    }
}

void* realloc(void* ptr, size_t size) {
    using namespace malloc_count;

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

            count_free(old_size);
            count_malloc(size);

            return (char*)new_ptr + sizeof(block_header_t);
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

#endif