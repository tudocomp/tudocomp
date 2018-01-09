#include <algorithm>
#include <iostream>
#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>

namespace tdc { namespace lcpcomp {
    constexpr len_t M = 1024 * 1024;
    using uint40_t = uint_t<40>;

	inline uint40_t swapBytes(uint40_t value) {
		uint40_t result = 0;
		result |= (value & 0xFF) << 32;
		result |= (value & 0xFF00) << 16;
		result |= (value & 0xFF0000);
		result |= (value & 0xFF000000) >> 16;
		result |= (value & (uint40_t)0xFF00000000) >> 32;
		return result;
	}

    inline void defactorize(const std::string& textfilename,
                          const std::string& outfilename,
                          const size_t threshold,
                          const len_t mb_ram) {      

		StatPhase phase("PLCPDeComp");
        StatPhase::wrap("Read Factors", [&]{
			
			std::cout << "Read from IntegerFileArray: " << std::endl;
					
			IntegerFileArray<uint40_t> text (textfilename.c_str());
			typedef IntegerFileArray<uint40_t> text_t;
			
			const size_t n = text.size();
			std::cout << "Number of factors: " << n / 2 << " (" << n << " components)" << std::endl;
			
			for (int i = 0; i < n; i++)
			{
				uint40_t pos = swapBytes(text[i++]);
				uint40_t len = swapBytes(text[i]);
				if(len == 0) {
					char c = char(pos);
					if(c == '\n') std::cout << "(\\n)";
					else std::cout << "(" << c << ")";
				} 
				else {
					std::cout << "(" << pos << ", " << len << ")";
				}	
			}	
			
			std::cout << std::endl;
			std::cout << "Read from BitIStream: " << std::endl;
			
			tdc::io::Path path(textfilename);
			tdc::io::Input inFile(static_cast<tdc::io::Path&&>(path));
			tdc::io::BitIStream in(inFile);
				
			
			while(!in.eof()) {
				uint40_t pos = in.read_int<uint40_t>();
				uint40_t len = in.read_int<uint40_t>();
				if(len == 0) {
					char c = char(pos);
					if(c == '\n') std::cout << "(\\n)";
					else std::cout << "(" << c << ")";
				} 
				else {
					std::cout << "(" << pos << ", " << len << ")";
				}	
			}	
			std::cout << std::endl;	
        });

	}
	
	
}}//ns


bool file_exist(const std::string& filepath) {
	std::ifstream file(filepath);
	return !file.fail();
}


int main(int argc, char** argv) {
	if(argc <= 2) {
		std::cerr << "Usage : " << argv[0]
                  << " <infile> <outfile> [threshold] [mb_ram]" << std::endl;
		return 1;
	}
	tdc::StatPhase root("Root");

	const std::string infile { argv[1] };
	const std::string outfile { argv[2] };
	if(!file_exist(infile)) {
		std::cerr << "Infile " << infile << " does not exist" << std::endl;
		return 1;
	}

    const size_t threshold = (argc >= 3) ? std::stoi(argv[3]) : 2;
    const tdc::len_t mb_ram = (argc >= 4) ? std::stoi(argv[4]) : 512;

	tdc::lcpcomp::defactorize(infile, outfile, threshold, mb_ram);

    return 0;
}

