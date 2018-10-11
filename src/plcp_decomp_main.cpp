#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhaseStxxl.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lcpcomp/decompress/PLCPDecompGenerator.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>
#include <stxxl/bits/io/syscall_file.h>
#include <tudocomp/compressors/lcpcomp/PLCPTypes.hpp>

bool file_exist(const std::string& filepath) {
    std::ifstream file(filepath);
    return !file.fail();
}


int main(int argc, char** argv) {
    if(argc <= 2) {
        std::cerr << "Usage : " << argv[0]
                  << " <infile> <outfile> [mb_ram]" << std::endl;
        return 1;
    }

    tdc::StatPhaseStxxlConfig statsCfg(false);
    statsCfg.parallelTime_combined = true;
    statsCfg.waitTime_combined = true;
    statsCfg.numberOfBytes_read = true;
    statsCfg.numberOfBytes_write = true;
    statsCfg = tdc::StatPhaseStxxlConfig();
    tdc::StatPhaseStxxl::enable(statsCfg);


    const std::string infile { argv[1] };
    const std::string outfile { argv[2] };
    if(!file_exist(infile)) {
        std::cerr << "Infile " << infile << " does not exist" << std::endl;
        return 1;
    }

    const uint64_t mb_ram = (argc >= 3) ? std::stoi(argv[3]) : 512;

    tdc::StatPhase root("Root");
    // generate stats instance
    stxxl::stats * Stats = stxxl::stats::get_instance();
    // start measurement here
    stxxl::stats_data stats_begin(*Stats);
    stxxl::block_manager * bm = stxxl::block_manager::get_instance();

    tdc::lcpcomp::vector_of_char_t textBuffer;
    {
        stxxl::syscall_file compressed_file(infile, stxxl::file::open_mode::RDONLY);
        tdc::lcpcomp::vector_of_upair_initial_t compressed(&compressed_file);

        textBuffer = tdc::lcpcomp::PLCPDecompGenerator::decompress(compressed, mb_ram);
    }

    // substract current stats from stats at the beginning of the measurement
    std::cout << (stxxl::stats_data(*Stats) - stats_begin);
    std::cout << "Maximum EM allocation: " << bm->get_maximum_allocation() << std::endl;
    root.log("EM Peak", bm->get_maximum_allocation());

    auto algorithm_stats = root.to_json();

    std::ofstream outputFile;
    outputFile.open(outfile);
    for (auto character : textBuffer)
        outputFile << character;
    outputFile.close();

    tdc::json::Object meta;
    meta.set("title", "TITLE");
    meta.set("startTime", "STARTTIME");

    meta.set("config", "NONE");
    meta.set("input", "FILENAME");;
    meta.set("inputSize", "INPUTSIZE");
    meta.set("output", "NONE");
    meta.set("outputSize", "OUTPUTSIZE");
    meta.set("rate", "NONE");

    tdc::json::Object stats;
    stats.set("meta", meta);
    stats.set("data", algorithm_stats);

    std::cout << "data for http://tudocomp.org/charter" << std::endl;
    stats.str(std::cout);
    std::cout << std::endl;

    return 0;
}

