#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdlib>

#ifndef STATS_DISABLED

namespace malloc_callback {
    void on_alloc(size_t);
    void on_free(size_t);
}

#endif

#ifndef __CYGWIN__ // this does not work in Cygwin

extern "C" void* __libc_malloc(size_t);
extern "C" void  __libc_free(void*);
extern "C" void* __libc_realloc(void*, size_t);

#endif

