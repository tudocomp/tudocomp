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

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#include <chrono>
#include <thread>

using namespace tdc;

TEST(stats, example) {
    StatPhase root("Root");

    // Phase 1
    StatPhase::wrap("Phase 1", []{
        char* alloc1 = new char[2048];
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        delete[] alloc1;
    });

    // Phase 2
    StatPhase::wrap("Phase 2", []{
        char* alloc2 = new char[3072];

        // Phase 2.1
        StatPhase::wrap("Phase 2.1", []{
            std::this_thread::sleep_for(std::chrono::milliseconds(30));

            StatPhase::log("A statistic", 147);
            StatPhase::log("Another statistic", 0.5);
        });

        // Phase 2.2
        StatPhase::wrap("Phase 2.2", []{
            char* alloc2_2 = new char[1024];
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            delete[] alloc2_2;
        });

        delete[] alloc2;
    });

    // Phase 3.X (looping)
    StatPhase::wrap("Phase 3", []{
        IntVector<uint32_t> vec(512, 0);
        
        StatPhase sub_phase("Init");
        int count = 0;
        while(!vec.empty()) {
            vec.pop_back();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if((++count % 128) == 0) {
                vec.shrink_to_fit();

                if(!vec.empty()) {
                    sub_phase.split(std::string("@") + std::to_string(vec.size()));
                    sub_phase.log_stat("remaining", vec.size());
                }
            }
        };
    });

    // Conclude tracking and print JSON to stdout
    root.to_json().str(std::cout);
}

