#include <malloc_count/malloc_count.hpp>

namespace malloc_count {
    #define NO_PHASE_ID 0
    #define PHASE_STACK_SIZE 16

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
    block->phase_id = (paused || pcur < 0) ? NO_PHASE_ID : pstack[pcur].id;
    block->size = size;

    if(!paused && pcur >= 0) {
        for(int i = 0; i <= pcur; i++) {
            pstack[i].mem_current += size;
            if(pstack[i].mem_current > pstack[i].mem_peak) {
                pstack[i].mem_peak = pstack[i].mem_current;
            }
        }
    }

    return (char*)ptr + sizeof(block_header_t);
}

void free(void* ptr) {
    using namespace malloc_count;

    if(!ptr) return;

    auto block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if(block->magic == MEMBLOCK_MAGIC) {
        if(!paused && pcur >= 0 /*&& block->phase_id >= pstack[0].id*/) {
            for(int i = 0; i <= pcur; i++) {
                //if(block->phase_id >= pstack[i].id) {
                    pstack[i].mem_current -= block->size;
                //}
            }
        }

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

#endif
