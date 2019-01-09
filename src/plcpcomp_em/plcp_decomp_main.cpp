#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/util/STXXLStatExtension.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
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

    const std::string infile { argv[1] };
    const std::string outfile { argv[2] };
    if(!file_exist(infile)) {
        std::cerr << "Infile " << infile << " does not exist" << std::endl;
        return 1;
    }

    const uint64_t mb_ram = (argc >= 3) ? std::atoi(argv[3]) : 512;

    tdc::StatPhase::register_extension<tdc::STXXLStatExtension>();
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

    std::cout << "data for http://tudocomp.org/charter" << std::endl;
    std::cout << algorithm_stats;
    std::cout << std::endl;

    return 0;
}

