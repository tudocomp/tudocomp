#include <algorithm>
#include <forward_list>
#include <iostream>
#include <unordered_map>

#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>

#include <stxxl/bits/io/syscall_file.h>

namespace tdc { namespace lcpcomp {
    template<typename node_t>
    class ReferenceGraph {
    private:
        std::vector<node_t> m_path;

        std::unordered_map<node_t, node_t> m_edges;
        size_t m_longest_path;
        size_t m_shortcuts;

        inline node_t follow_path(const node_t start, const node_t avoid) {
            auto v = start;

            // follow and store path
            auto it = m_edges.find(v);
            while(it != m_edges.end()) {
                m_path.emplace_back(v);

                v = it->second;
                CHECK(v != start) << "cycle detected";
                CHECK(v != avoid) << "cycle detected";

                it = m_edges.find(v);
            }

            if(m_path.size() > 1) {
                // create shortcuts
                m_shortcuts += m_path.size() - 1;
                for(auto u : m_path) {
                    m_edges[u] = v;
                }
            }

            m_longest_path = std::max(m_longest_path, m_path.size());
            m_path.clear(); // clear, but keep allocated

            return v;
        }

    public:
        inline ReferenceGraph() : m_longest_path(0), m_shortcuts(0) {
        }

        inline size_t longest_path() const {
            return m_longest_path;
        }

        inline size_t num_shortcuts() const {
            return m_shortcuts;
        }

        inline size_t num_edges() const {
            return m_edges.size();
        }

        // insert an edge, throw error if a cycle is detected or if u already
        // has an outgoing edge
        inline void insert_edge(node_t u, node_t v) {
            CHECK(m_edges.find(u) == m_edges.end()) << "duplicate edge detected";
            m_edges[u] = follow_path(v, u);
        }
    };

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
    using filemapped_vector_t = stxxl::VECTOR_GENERATOR<
        uint40_t, blocks_per_page, cache_pages, block_size>::result;

    inline void factorize(const std::string& textfilename,
                          const std::string& outfilename,
                          const size_t threshold,
                          const len_t mb_ram) {

		StatPhase phase("PLCPComp");

        stxxl::syscall_file sa_file(textfilename + ".sa5", stxxl::file::open_mode::RDONLY);
        filemapped_vector_t sa(&sa_file);

        stxxl::syscall_file isa_file(textfilename + ".isa5", stxxl::file::open_mode::RDONLY);
        filemapped_vector_t isa(&isa_file);

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

        LOG(INFO) << "Start verification... factors: " << refs.size();
        StatPhase::wrap("Verify", [&]{
            ReferenceGraph<size_t> g;
            const size_t n = sa.size();
            size_t num_replaced = 0;
            size_t next_unreplaced = 0;

            decltype(refs)::backing_vector_type::bufreader_type reader(refs.factors);
            for(auto& factor : reader) {
                const size_t fpos = factor.pos;
                const size_t fsrc = factor.src;
                const size_t flen = factor.len;
                num_replaced += flen;

                CHECK_LT(num_replaced, n) << "more symbols replaced than input has symbols";
                CHECK_GE(fpos, next_unreplaced) << "factors are interleaving";
                CHECK_LT(fsrc, n) << "factor start out of bounds";
                CHECK_LT(fsrc + flen - 1, n) << "factor end out of bounds";

                next_unreplaced = fpos + flen;

                for(size_t k = 0; k < flen; k++) {
                    g.insert_edge(fpos + k, fsrc + k);
                }


            }

            StatPhase::log("longest_path", g.longest_path());
            StatPhase::log("num_edges", g.num_edges());
            StatPhase::log("num_shortcuts", g.num_shortcuts());
        });
        LOG(INFO) << "Done!";

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
                    while(p++ < factor.pos) {
                        bw.write(ins.get());
                        bw.write(0);
                    }

                    // encode factor
                    bw.write(factor.src);
                    bw.write(factor.len);

                    p += size_t(factor.len);
                    skip_bytes(ins, factor.len);
                    num_replaced += size_t(factor.len);
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

