#include <algorithm>
#include <iostream>
#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>

constexpr size_t THRESHOLD = 2;

namespace tdc { namespace lcpcomp {
    constexpr len_t M = 1024 * 1024;
    constexpr len_t MEMORY = 512 * M;

    inline void factorize(const std::string& textfilename, const std::string& outfilename, size_t threshold) {
        using uint40_t = uint_t<40>;

		StatPhase phase("PLCPComp");
		IntegerFileArray<uint40_t> sa  ((textfilename + ".sa5").c_str());
		IntegerFileArray<uint40_t> isa ((textfilename + ".isa5").c_str());
		IntegerFileArray<char> text (textfilename.c_str());
        typedef IntegerFileArray<char> text_t;

		PLCPFileForwardIterator pplcp    ((textfilename + ".plcp").c_str());

		RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa,MEMORY);
		StatPhase::wrap("Search Peaks", [&]{
				compute_references(filesize(textfilename.c_str())-1, refStrategy, pplcp, threshold);
		});

        tdc::lzss::FactorBufferDisk refs;
		StatPhase::wrap("Compute References", [&]{
			refStrategy.factorize(refs);
            StatPhase::log("num_factors", refs.size());
    	});

        StatPhase::wrap("Encode Factors", [&]{
            /*
            tdc::Output output(tdc::Path(outfilename), true);
            tdc::Env env = tdc::create_env(Meta("plcpcomp", "plcp"));
            tdc::HuffmanCoder::Encoder coder(std::move(env), output, tdc::lzss::TextLiterals<text_t,decltype(refs)>(text, refs));
            tdc::lzss::encode_text(coder, text, refs);
            */

            // Use the same stupid encoding as EM-LPF for a comparison
            tdc::Output output(tdc::Path(outfilename), true);
            tdc::BitOStream out(output);           

            // walk over factors
            size_t p = 0; //! current text position
            for(auto& factor : refs) {
                // encode literals until cursor reaches factor
                while(p < factor.pos) {
                    out.write_int(uint40_t(text[p++]));
                    out.write_int(uint40_t(0));
                }

                // encode factor
                out.write_int(uint40_t(factor.src));
                out.write_int(uint40_t(factor.len));
                p += size_t(factor.len);
            }

            const size_t n = text.size();
            while(p < n)  {
                // encode remaining literals
                out.write_int(uint40_t(text[p++]));
                out.write_int(uint40_t(0));
            }
        });

	}
}}//ns


bool file_exist(const std::string& filepath) {
	std::ifstream file(filepath);
	return !file.fail();
}


int main(int argc, char** argv) {
	if(argc <= 2) {
		std::cerr << "Usage : " << argv[0] << " <infile> <outfile>" << std::endl;
		return 1;
	}
	tdc::StatPhase root("Root");

	const std::string infile { argv[1] };
	const std::string outfile { argv[2] };
	if(!file_exist(infile)) {
		std::cerr << "Infile " << infile << " does not exist" << std::endl;
		return 1;
	}
	if(!file_exist(infile + ".sa5")) {
		std::cerr << "Infile " << infile << ".sa5 does not exist" << std::endl;
		return 1;
	}
	if(!file_exist(infile + ".isa5")) {
		std::cerr << "Infile " << infile << ".isa5 does not exist" << std::endl;
		return 1;
	}
	if(!file_exist(infile + ".plcp")) {
		std::cerr << "Infile " << infile << ".plcp does not exist" << std::endl;
		return 1;
	}

	tdc::lcpcomp::factorize(infile, outfile, THRESHOLD);

	root.to_json().str(std::cout);
	std::cout << std::endl;

    return 0;
}

