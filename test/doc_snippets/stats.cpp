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

    // Phase 3 (splitting)
    StatPhase::wrap("Phase 3", []{
        // Phase 3.1 yields a complex result
        StatPhase sub_phase("Phase 3.1");

        char* result_part_1 = new char[1024];
        char* result_part_2 = new char[2048];
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        // Phase 3.2 may use the result
        sub_phase.split("Phase 3.2");
        delete[] result_part_2;
        delete[] result_part_1;
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    });

    // Print data in JSON representation to stdout
    std::cout << root.to_json() << std::endl;
}

TEST(stats, pause_resume) {
    StatPhase phase("Pause and Resume");

    // Allocate memory, but only track mem2
    StatPhase::pause_tracking();
    char* mem1 = new char[1024];
    StatPhase::resume_tracking();

    char* mem2 = new char[2048];

    // Sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Free memory, but only track mem2
    StatPhase::pause_tracking();
    delete[] mem1;
    StatPhase::resume_tracking();

    delete[] mem2;

    // Print data in JSON representation to stdout
    std::cout << phase.to_json() << std::endl;
}

