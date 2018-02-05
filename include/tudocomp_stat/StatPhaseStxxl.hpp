#pragma once

#ifndef STATS_DISABLED

#include <stxxl/bits/io/iostats.h>
#include <tudocomp_stat/StatPhase.hpp>

#define EM_HEADER "STXXL "

namespace tdc {

struct StatPhaseStxxlConfig {
    
    StatPhaseStxxlConfig(bool value = true) {
        numberOfBlocks_read =
        numberOfBlocks_write =
        numberOfBytes_read =
        numberOfBytes_write =
        avgBlockSize_read =
        avgBlockSize_write =
        time_read =
        time_write =
        parallelTime_read =
        parallelTime_write =
        parallelTime_combined =
        waitTime_read =
        waitTime_write =
        waitTime_combined =
        value;
    }
    
    bool numberOfBlocks_read;
    bool numberOfBlocks_write;
    bool numberOfBytes_read;
    bool numberOfBytes_write;
    bool avgBlockSize_read;
    bool avgBlockSize_write;
    bool time_read;
    bool time_write;
    bool parallelTime_read;
    bool parallelTime_write;
    bool parallelTime_combined;
    bool waitTime_read;
    bool waitTime_write;
    bool waitTime_combined;
};

class StatPhaseStxxl {
private:    
    static stxxl::stats* s_stxxl_stats;    
    static StatPhaseStxxlConfig s_config;

    static void onStart(PhaseData* phaseData) {
        phaseData->stxxl_stats_begin = 
            new stxxl::stats_data(*s_stxxl_stats);
    }
    
    static void onStop(PhaseData* phaseData) { 
        delete static_cast<stxxl::stats_data*>(phaseData->stxxl_stats_begin);
    }
    
    static void onRead(PhaseData* phaseData) {
        stxxl::stats_data *stats_begin = 
            static_cast<stxxl::stats_data *>(phaseData->stxxl_stats_begin);
        stxxl::stats_data stats_result =
            stxxl::stats_data(*s_stxxl_stats) - *stats_begin;
        
        if(phaseData->first_stat_stxxl) {
            delete phaseData->first_stat_stxxl;
            phaseData->first_stat_stxxl = nullptr;
        }        

        if(s_config.numberOfBlocks_read)
            phaseData->log_stat_stxxl(
                EM_HEADER "Number of reads", 
                stats_result.get_reads ());
                
        if(s_config.avgBlockSize_read)
            phaseData->log_stat_stxxl(
                EM_HEADER "Average block size (read)", 
                stats_result.get_read_volume () / 
                double(stats_result.get_reads ()));
                
        if(s_config.numberOfBytes_read)
            phaseData->log_stat_stxxl(
                EM_HEADER "Total Bytes (read)", 
                stats_result.get_read_volume ());
                
        if(s_config.time_read)
            phaseData->log_stat_stxxl(
                EM_HEADER "Time spent (read)", 
                stats_result.get_read_time ());
                
        if(s_config.parallelTime_read)
            phaseData->log_stat_stxxl(
                EM_HEADER "Time spent (parallel read)", 
                stats_result.get_pread_time ());
                
        if(s_config.waitTime_read)
            phaseData->log_stat_stxxl(
                EM_HEADER "Wait time (read)", 
                stats_result.get_wait_read_time ());


        if(s_config.numberOfBlocks_write)
            phaseData->log_stat_stxxl(
                EM_HEADER "Number of writes", 
                stats_result.get_writes ());
                
        if(s_config.avgBlockSize_write)
            phaseData->log_stat_stxxl(
                EM_HEADER "Average block size (write)", 
                stats_result.get_written_volume () / 
                double(stats_result.get_writes ()));
                
        if(s_config.numberOfBytes_write)
            phaseData->log_stat_stxxl(
                EM_HEADER "Total Bytes (write)", 
                stats_result.get_written_volume ());
                
        if(s_config.time_write)
            phaseData->log_stat_stxxl(
                EM_HEADER "Time spent (write)", 
                stats_result.get_write_time ());
                
        if(s_config.parallelTime_write)
            phaseData->log_stat_stxxl(
                EM_HEADER "Time spent (parallel write)", 
                stats_result.get_pwrite_time ());
                
        if(s_config.waitTime_write)
            phaseData->log_stat_stxxl(
                EM_HEADER "Wait time (write)", 
                stats_result.get_wait_write_time ());
                
                  
                
        if(s_config.parallelTime_combined)
            phaseData->log_stat_stxxl(
                EM_HEADER "Time spent (read/write)", 
                stats_result.get_pio_time ());
                
        if(s_config.waitTime_combined)
            phaseData->log_stat_stxxl(
                EM_HEADER "Wait time (read/write)", 
                stats_result.get_io_wait_time ());

    }
    
public:
    inline static void enable(StatPhaseStxxlConfig config) {
        StatPhaseStxxl::s_config = config;
        enable();
    }
    
    inline static void enable() {
        PhaseData::s_stxxl_start = &onStart;
        PhaseData::s_stxxl_read_stats = &onRead;
        PhaseData::s_stxxl_stop = &onStop;
    }
    
    inline static void disable() {
        PhaseData::s_stxxl_start = nullptr;
        PhaseData::s_stxxl_read_stats = nullptr;
        PhaseData::s_stxxl_stop = nullptr;
    }
};

stxxl::stats* StatPhaseStxxl::s_stxxl_stats = stxxl::stats::get_instance();
StatPhaseStxxlConfig StatPhaseStxxl::s_config = StatPhaseStxxlConfig();
}

#else

namespace tdc {

class StatPhaseStxxl {
public:
    inline static void enable(StatPhaseStxxlConfig config) {}
    inline static void enable() {}
    inline static void disable() {}
};
}

}

#endif

