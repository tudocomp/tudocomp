#pragma once

#include <tudocomp/config.h>

#ifdef STXXL_FOUND

#include <tudocomp_stat/StatPhaseExtension.hpp>
#include <stxxl/bits/io/iostats.h>

namespace tdc {

class STXXLStatExtension : public StatPhaseExtension {
private:
    static stxxl::stats* s_stats;

    static inline stxxl::stats_data sample() {
        return stxxl::stats_data(*s_stats);
    }

    stxxl::stats_data m_begin;

public:
    inline STXXLStatExtension() : m_begin(sample()) {
    }

    virtual inline void write(json& data) override {
        auto stats = sample() - m_begin;

        // TODO: once the charter supports displaying objects,
        //       organize these into an "stxxl" object
        data["cached_read_volume"]    = stats.get_cached_read_volume();
        data["cached_reads"]          = stats.get_cached_reads();
        data["cached_writes"]         = stats.get_cached_writes();
        data["cached_written_volume"] = stats.get_cached_written_volume();
        data["io_wait_time"]          = stats.get_io_wait_time();
        data["pio_time"]              = stats.get_pio_time();
        data["pread_time"]            = stats.get_pread_time();
        data["pwrite_time"]           = stats.get_pwrite_time();
        data["read_time"]             = stats.get_read_time();
        data["read_volume"]           = stats.get_read_volume();
        data["reads"]                 = stats.get_reads();
        data["wait_read_time"]        = stats.get_wait_read_time();
        data["wait_write_time"]       = stats.get_wait_write_time();
        data["write_time"]            = stats.get_write_time();
        data["writes"]                = stats.get_writes();
        data["written_volume"]        = stats.get_written_volume();
    }
};

}

#endif // STXXL_FOUND
