/**
 *
 * This file contains code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/tudocomp.hpp>

#include <chrono>
#include <thread>

using namespace tdc;

using ms = std::chrono::milliseconds;

TEST(stats, example) {
    Env env = create_env(Meta("undisclosed", "test", "Test environment"));

    // Phase 1
    env.begin_stat_phase("Phase 1");
    {
        char* alloc1 = new char[2048];
        std::this_thread::sleep_for(ms(30));
        delete[] alloc1;
    }
    env.end_stat_phase();

    // Phase 2
    env.begin_stat_phase("Phase 2");
    {
        char* alloc2 = new char[3072];

        // Phase 2.1
        env.begin_stat_phase("Phase 2.1");
        {
            std::this_thread::sleep_for(ms(30));
            env.log_stat("A statistic", 147);
            env.log_stat("Another statistic", 0.5);
        }
        env.end_stat_phase();

        // Phase 2.2
        env.begin_stat_phase("Phase 2.2");
        {
            char* alloc2_2 = new char[1024];
            std::this_thread::sleep_for(ms(40));
            delete[] alloc2_2;
        }
        env.end_stat_phase();

        delete[] alloc2;
    }
    env.end_stat_phase();

    // Conclude tracking and print JSON to stdout
    env.finish_stats().to_json().str(/*std::cout - uncomment me*/);
}

