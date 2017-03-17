#include <tudocomp_stat/malloc.hpp>
#include <tudocomp_stat/StatPhase.hpp>

using tdc::StatPhase;

StatPhase* StatPhase::s_current = nullptr;

void malloc_callback::on_alloc(size_t bytes) {
    StatPhase::track_alloc(bytes);
}

void malloc_callback::on_free(size_t bytes) {
    StatPhase::track_free(bytes);
}
