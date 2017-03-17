#include <tudocomp_stat/Stat.hpp>

using tdc::Stat;

Stat* Stat::s_current = nullptr;

void malloc_callback::on_alloc(size_t bytes) {
    Stat::track_alloc(bytes);
}

void malloc_callback::on_free(size_t bytes) {
    Stat::track_free(bytes);
}
