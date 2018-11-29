#include <tudocomp/config.h>
#ifdef STXXL_FOUND

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include "test/util.hpp"

#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/compressors/lzss/UnreplacedLiterals.hpp>

#include <tudocomp/coders/ASCIICoder.hpp>
#include <tudocomp/compressors/lzss/BufferedBidirectionalCoder.hpp>
#include <tudocomp/compressors/lzss/UnreplacedLiterals.hpp>

#include <stxxl/bits/io/syscall_file.h>

using namespace tdc;
using namespace tdc::lcpcomp;
using namespace tdc::lzss;
using io::InputView;

using test_coder_t = BufferedBidirectionalCoder<ASCIICoder, ASCIICoder, ASCIICoder>;
using uint40_t = uint_t<40>;

// parameters for filemapped vector
// these are "good" values, determined by trial and error on elbait
constexpr size_t blocks_per_page = 4;
constexpr size_t cache_pages = 8;
constexpr size_t block_size = 16 * 4096 * sizeof(uint40_t);

using uint40_pair_t = std::pair<uint40_t, uint40_t>;
using filemapped_phi_array_t = stxxl::VECTOR_GENERATOR<
    uint40_pair_t, blocks_per_page, cache_pages, block_size>::result;

template<class text_t>
void diskstrategy(text_t& text, size_t threshold, lzss::FactorBufferDisk& refs) {
    DVLOG(2) << "diskstrategy for text of size " << text.size();
    DCHECK_EQ(0, text[text.size()-1]);

	text.require(ds::SA | ds::PHI);

    char* phifile { new char[TMP_MAX] };
    char* plcpfile { new char[TMP_MAX] };

    std::tmpnam(phifile);
    std::tmpnam(plcpfile);

    // map file for phi
    stxxl::syscall_file f_phi(phifile,
        stxxl::file::open_mode::CREAT | stxxl::file::open_mode::RDWR);

    {
        filemapped_phi_array_t phi(&f_phi);
        size_t num_irreducible = 0;

        // construct index DS
        {
            DVLOG(2) << "compute PHI";
            const auto& tphi = text.require_phi();

            uint40_t prev_phi = uint40_t(tphi[0]);
            phi.push_back(uint40_pair_t(0, prev_phi));
            num_irreducible = 1;

            for(uint40_t i = 1; i < tphi.size(); i++) {
                auto phi_i = uint40_t(tphi[i]);

                if(phi_i != prev_phi+1) {
                    // irreducible entry
                    phi.push_back(uint40_pair_t(i, phi_i));
                    ++num_irreducible;
                }

                prev_phi = phi_i;
            }
        }
        {
            DVLOG(2) << "compute PLCP";
            std::ofstream plcpo(plcpfile);
       		construct_plcp_bitvector(text.require_sa(), text).write_data(plcpo);
        }
        {
            DVLOG(2) << "map STXXL vectors";
            // check index ds
            {
                DVLOG(2) << "check index ds";
                PLCPFileForwardIterator pplcp(plcpfile);
                const auto& tphi = text.require_phi();
                const auto& plcp = text.require_plcp();

                DCHECK_EQ(0ULL, phi[0].first);
                uint64_t irreducible_pos = 0;
                uint64_t irreducible_phi = phi[0].second;
                size_t j = 1;

                for(size_t i = 1; i < tphi.size(); i++) {
                    if(j < num_irreducible && i == uint64_t(phi[j].first)) {
                        irreducible_pos = phi[j].first;
                        irreducible_phi = phi[j].second;
                        ++j;
                    }

                    DCHECK_EQ(
                        uint64_t(tphi[i]),
                        irreducible_phi + (i - irreducible_pos));
                }

                for(size_t i = 0; i < plcp.size()-1; ++i) {
                    DCHECK_EQ(pplcp(),(uint64_t) plcp[i]);
                    pplcp.advance();
                }
            }

            {
                DVLOG(2) << "factorize";
         		PLCPFileForwardIterator pplcp(plcpfile);
	            RefDiskStrategy<decltype(phi)> refStrategy(phi);
        	    compute_references(text.size(), refStrategy, pplcp, threshold);
	            refStrategy.factorize(refs);
            }
        }
    }

    DVLOG(2) << "clean up";
    f_phi.close_remove();
    delete [] phifile;
    delete [] plcpfile;

    DVLOG(2) << "done (refs: " << refs.size() << ")";
}

template<class text_t>
void test_plcp(text_t& text) {
    using namespace tdc::lcpcomp;

    lzss::FactorBufferRAM bufRAM;
    {
        PLCPStrategy strat(PLCPStrategy::meta().config());
        strat.factorize(text, 2, bufRAM);
    }

    lzss::FactorBufferDisk bufDisk;
    {
        diskstrategy(text, 2, bufDisk);
    }

    DCHECK_EQ(bufRAM.size(), bufDisk.size());
    {
        DVLOG(2) << "references:";
        auto itRAM = bufRAM.begin();
        auto itDisk = bufDisk.begin();
        while(itRAM != bufRAM.end() && itDisk != bufDisk.end()) {
            DVLOG(2) << "(" << itDisk->pos << "," << itDisk->src << "," << itDisk->len << ")";
            ASSERT_EQ(itRAM->pos, itDisk->pos);
            ASSERT_EQ(itRAM->src, itDisk->src);
            ASSERT_EQ(itRAM->len, itDisk->len);
            ++itRAM;
            ++itDisk;
        }
    }
    std::vector<uint8_t> outbufDisk;
    std::vector<uint8_t> outbufRAM;
    {
        tdc::Output outputRAM(outbufRAM);
        auto coder = test_coder_t(test_coder_t::meta().config()).encoder(
            outputRAM, tdc::lzss::UnreplacedLiterals<text_t,decltype(bufRAM)>(text, bufRAM));

        coder.encode_text(text, bufRAM);
    }
    {
        tdc::Output outputDisk(outbufDisk);
        auto coder = test_coder_t(test_coder_t::meta().config()).encoder(
            outputDisk, tdc::lzss::UnreplacedLiterals<text_t,decltype(bufDisk)>(text, bufDisk));

        coder.encode_text(text, bufDisk);
    }

    DCHECK_EQ(outbufDisk.size(), outbufRAM.size());

    {
        auto itRAM = outbufRAM.begin();
        auto itDisk = outbufDisk.begin();
        while(itRAM != outbufRAM.end() && itDisk != outbufDisk.end()) {
            ASSERT_EQ(*itRAM, *itDisk);
            ++itRAM;
            ++itDisk;
        }
    }

}


template<class textds_t>
class TestRunner {
	void (*m_testfunc)(textds_t&);
	public:
	TestRunner(void (*testfunc)(textds_t&))
		: m_testfunc(testfunc) {}

	void operator()(const std::string& str) {
		DLOG(INFO) << "str = \"" << str << "\"" << " size: " << str.length();
		test::TestInput input = test::compress_input(str);
		InputView in = input.as_view();
		DCHECK_EQ(str.length()+1, in.size());
		textds_t t = Algorithm::instance<textds_t>(in);
		DCHECK_EQ(str.length()+1, t.size());
		m_testfunc(t);
	}


};

// TEST(plcp, easy) {
//     TestRunner<TextDS<>> runner(test_plcp);
//     runner("aaaaaaaaa");
// }

#define TEST_DS_STRINGCOLLECTION(func) \
	TestRunner<TextDS<>> runner(func); \
	test::roundtrip_batch(runner); \
	test::on_string_generators(runner,8);
TEST(plcp, filecheck)          { TEST_DS_STRINGCOLLECTION(test_plcp); }
#undef TEST_DS_STRINGCOLLECTION

#endif
