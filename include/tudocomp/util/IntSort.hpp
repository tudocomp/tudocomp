#pragma once

#include <limits>
#include <array>
#include <vector>
#include <tuple>
#include <algorithm>
#include <type_traits>

#include <tudocomp/util.hpp>

#ifdef ENABLE_OPENMP
#include <omp.h>
#endif

template<typename Iter, typename KeyExtract, typename Key, size_t RADIX_WIDTH=8,
        typename T = typename std::iterator_traits<Iter>::value_type >
inline std::pair<std::vector<T>, bool> intsort_impl(const Iter begin, const Iter end, KeyExtract key_extract,
                    const Key max_key = std::numeric_limits<Key>::max()) {
    const size_t n = std::distance(begin, end);
    std::vector<T> buffer(n);

    if (n < 2 || max_key < 1) return {buffer, false}; // in these cases the input is trivially sorted


    // compute mask and shifts to later compute the queue index
    static_assert(RADIX_WIDTH >= 1, "Radix has to be at least 2");
    static_assert(RADIX_WIDTH <= 8*sizeof(T), "Radix is not allowed to exceed numer of bits in T");

    constexpr size_t no_queues = 1 << RADIX_WIDTH;
    constexpr Key mask = (Key(1) << RADIX_WIDTH) - 1;
    auto get_queue_index = [&](Key key, int iteration) {return (key >> (iteration * RADIX_WIDTH)) & mask;};

    // setup strucuture to compute queue boundaries
    int max_threads = 1;
#ifdef ENABLE_OPENMP
    max_threads = std::min<int>(omp_get_max_threads(), tdc::idiv_ceil(n, 1 << 17));
#endif
    using IndexArray = std::array<size_t, no_queues>;
    std::vector< IndexArray > thread_counters(2 * max_threads);

    // compute how many iterations we need to sort numbers [0, ..., max_key], i.e. log(max_key, base=RADIX_WIDTH)
    int no_iters = 1;
    {
        Key key = max_key;
        while(key >>= RADIX_WIDTH) no_iters++;
    }

#ifdef ENABLE_OPENMP
#pragma omp parallel num_threads(max_threads)
    {
        // symmetry breaking
        const auto tid = omp_get_thread_num();
        const auto no_threads = omp_get_num_threads();
        DCHECK_GE(max_threads, no_threads);
#else
    {
        constexpr int tid = 0;
        constexpr int no_thrads = 1;
#endif

        // figure out workload for each thread
        const size_t chunk_size = tdc::idiv_ceil(n, no_threads);
        const auto chunk = std::make_pair(chunk_size * tid, std::min(chunk_size * (tid + 1), n));

        // thread-local counters and iterators
        IndexArray queue_pointer;

        auto input_base  = begin;
        auto buffer_base = buffer.begin();

        for(int iteration = 0; iteration < no_iters; ++iteration) {
            const auto input_begin = input_base + chunk.first;
            const auto input_end   = input_base + chunk.second;

#ifdef ENABLE_OPENMP
            if (iteration) {
                #pragma omp barrier
            }
#endif

            auto &counters = thread_counters[tid];
            counters.fill(0);
            for (auto it = input_begin; it != input_end; ++it) {
                counters[get_queue_index(key_extract(*it), iteration)]++;
            }


#ifdef ENABLE_OPENMP
#pragma omp barrier
#endif
            {
                size_t index = 0;
                size_t tmp;
                for (size_t qid = 0; qid != no_queues; ++qid) {
                    for (int ttid = 0; ttid < no_threads; ttid++) {
                        if (ttid == tid) tmp = index;
                        index += thread_counters[ttid][qid];
                    }
                    queue_pointer[qid] = tmp;
                }
            }

            for (auto it = input_begin; it != input_end; ++it) {
                const Key key = key_extract(*it);
                const auto index = queue_pointer[get_queue_index(key, iteration)]++;

                buffer_base[index] = std::move(*it);
            }

            std::swap(input_base,  buffer_base);
        }
    }

    return {buffer, no_iters % 2};
}

template<typename Iter, typename KeyExtract, typename Key, size_t RADIX_WIDTH=8,
        typename T = typename std::iterator_traits<Iter>::value_type >
inline void intsort(const Iter begin, const Iter end, KeyExtract key_extract,
                         const Key max_key = std::numeric_limits<Key>::max()) {
    auto ret = intsort_impl<Iter, KeyExtract, Key, RADIX_WIDTH, T>(begin, end, key_extract, max_key);

    if (ret.second)
        std::copy(ret.first.cbegin(), ret.first.cend(), begin);
}

template<typename T, typename KeyExtract, typename Key, size_t RADIX_WIDTH=8>
inline void intsort(std::vector<T>& input, KeyExtract key_extract, const Key max_key = std::numeric_limits<Key>::max()) {
    auto begin = input.begin();
    auto end = input.end();

    auto ret = intsort_impl<decltype(begin), KeyExtract, Key, RADIX_WIDTH, T>(begin, end, key_extract, max_key);

    if (ret.second) input.swap(ret.first);
}