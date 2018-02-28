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

    inline void factorize(const std::string& textfilename,
                          const std::string& outfilename,
                          const size_t threshold,
                          const len_t mb_ram) {

		StatPhase phase("PLCPComp");
		IntegerFileArray<uint40_t> sa  ((textfilename + ".sa5").c_str());
		IntegerFileArray<uint40_t> isa ((textfilename + ".isa5").c_str());
		PLCPFileForwardIterator pplcp  ((textfilename + ".plcp").c_str());

		RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa,mb_ram * M);
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
                for(auto& factor : refs) {
                    // encode literals until cursor reaches factor
                    while(p++ < factor.pos) {
                        bw.write(ins.get());
                        bw.write(0);
                    }

                    // encode factor
                    bw.write(factor.src);
                    bw.write(factor.len);

                    p += size_t(factor.len);
                    skip_bytes(ins, factor.len);
                    num_replaced += factor.len;
                }

                while(!ins.eof())  {
                    // encode remaining literals
                    bw.write(ins.get());
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

    const size_t threshold = (argc >= 3) ? std::stoi(argv[3]) : 2;
    const tdc::len_t mb_ram = (argc >= 4) ? std::stoi(argv[4]) : 512;

	tdc::lcpcomp::factorize(infile, outfile, threshold, mb_ram);

	root.to_json().str(std::cout);
	std::cout << std::endl;

    return 0;
}

