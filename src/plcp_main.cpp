#include <algorithm>
#include <forward_list>
#include <iostream>
#include <unordered_map>

#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp_stat/StatPhaseStxxl.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>

#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>

#include <stxxl/bits/io/syscall_file.h>

namespace tdc { namespace lcpcomp {
    constexpr len_t M = 1024 * 1024;
    using uint40_t = uint_t<40>;

    template<typename T>
    class BufferedWriter {
        private:
            std::ostream* m_outs;

            T* m_buf;
            size_t m_bufsize;

            size_t m_num;

            inline void flush() {
                m_outs->write((const char*)m_buf, m_num * sizeof(T));
                m_num = 0;
            }

        public:
            inline BufferedWriter(std::ostream& outs, size_t bufsize)
                : m_outs(&outs), m_bufsize(bufsize), m_num(0) {

                m_buf = new T[bufsize];
            }

            inline ~BufferedWriter() {
                flush();
                delete[] m_buf;
            }

            inline void write(const T& value) {
                m_buf[m_num++] = value;
                if(m_num >= m_bufsize) flush();
            }
    };

    inline void skip_bytes(std::istream& ins, size_t num) {
        while(num--) {
            ins.get();
        }
    }

    // parameters for filemapped vector
    // these are "good" values, determined by trial and error on elbait
    constexpr size_t blocks_per_page = 4;
    constexpr size_t cache_pages = 8;
    constexpr size_t block_size = 16 * 4096 * sizeof(uint40_t);

    using phi_array_t = stxxl::VECTOR_GENERATOR<
        std::pair<uint40_t, uint40_t>, blocks_per_page, cache_pages, block_size>::result;

    inline void factorize(const std::string& textfilename,
                          const std::string& outfilename,
                          const size_t threshold,
                          const len_t mb_ram) {

        StatPhaseStxxl::enable(StatPhaseStxxlConfig()); // enable all STXXL stats
		StatPhase phase("PLCPComp");

        stxxl::syscall_file phi_file(textfilename + ".phi5", stxxl::file::open_mode::RDONLY);
        phi_array_t phi(&phi_file);

		PLCPFileForwardIterator pplcp((textfilename + ".plcp").c_str());

		RefDiskStrategy<decltype(phi)> refStrategy(phi);
		StatPhase::wrap("Search Peaks", [&]{
            compute_references(filesize(textfilename.c_str())-1, refStrategy, pplcp, threshold);
		});

        tdc::lzss::FactorBufferDisk refs;
		StatPhase::wrap("Compute References", [&]{
			refStrategy.factorize(refs);
            StatPhase::log("num_factors", refs.size());
    	});

        StatPhase::wrap("Encode Factors", [&]{
            // Open input file
            auto input = tdc::Input(tdc::Path(textfilename));
            auto ins = input.as_stream();

            // Use the same stupid encoding as EM-LPF for a comparison
            auto output = tdc::Output(tdc::Path(outfilename), true);
            auto outs = output.as_stream();

            // walk over factors
            size_t num_replaced = 0;

            {
                BufferedWriter<uint40_t> bw(outs, 400);

                size_t p = 0; //! current text position

                decltype(refs)::backing_vector_type::bufreader_type reader(refs.factors);
                for(auto& factor : reader) {
                    // encode literals until cursor reaches factor
                    while(p < factor.pos) {
                        bw.write(ins.get());
                        bw.write(0);
                        ++p;
                    }

                    // encode factor
                    bw.write(len_t(factor.src));
                    bw.write(len_t(factor.len));

                    p += size_t(factor.len);
                    skip_bytes(ins, factor.len);
                    num_replaced += size_t(factor.len);
                }

                // encode remaining literals
                for(auto c = ins.get(); ins.good(); c = ins.get()){
                    bw.write(c);
                    bw.write(0);
                }
            }

            StatPhase::log("num_replaced", num_replaced);
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
	if(!file_exist(infile + ".phi5")) {
		std::cerr << "Infile " << infile << ".phi5 does not exist" << std::endl;
		return 1;
	}
	if(!file_exist(infile + ".plcp")) {
		std::cerr << "Infile " << infile << ".plcp does not exist" << std::endl;
		return 1;
	}
	{
		const size_t filesize = tdc::lcpcomp::filesize(infile.c_str());
		if(static_cast<size_t>(static_cast<tdc::len_compact_t>(filesize)) != filesize) {
			std::cerr << "The file " << infile << " is too large to handle. This program is compiled with " << sizeof(tdc::len_compact_t)*8 << "-bit address ranges." << std::endl;
		}
	}

    const size_t threshold = (argc >= 3) ? std::stoi(argv[3]) : 2;
    const tdc::len_t mb_ram = (argc >= 4) ? std::stoi(argv[4]) : 512;

	tdc::lcpcomp::factorize(infile, outfile, threshold, mb_ram);

	root.to_json().str(std::cout);
	std::cout << std::endl;

    return 0;
}

