#include <cstdlib>
#include <iostream>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/Rank.hpp>
#include <tudocomp/ds/Select.hpp>

int main(int argc, const char** argv) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << ": <n>" << std::endl;
        return 1;
    }

    // Get bit vector length from command-line parameter
    const size_t n = std::atoi(argv[1]);

    // Construct a bit vector where every second bit is set
    tdc::BitVector bv(n);
    for(size_t i = 1; i < n; i += 2) bv[i] = 1;

    // Construct Rank and Select data structures
    tdc::Rank    rank(bv);
    tdc::Select0 select0(bv);
    tdc::Select1 select1(bv);

    // Query the amount of 0-bits and 1-bits in bv using rank
    auto num0 = rank.rank0(n - 1);
    std::cout << "bv contains " << num0 << " 0-bits" << std::endl;

    auto num1 = rank.rank1(n - 1);
    std::cout << "bv contains " << num1 << " 1-bits" << std::endl;

    // Find positions of the last 0-bit and the last 1-bit in bv using select
    auto lastpos0 = select0(num0);
    std::cout << "the last 0 is located at position " << lastpos0 << std::endl;

    auto lastpos1 = select1(num1);
    std::cout << "the last 1 is located at position " << lastpos1 << std::endl;

    // Exit
    return 0;
}

