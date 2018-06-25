#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
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

// parameters for filemapped vector
// these are "good" values, determined by trial and error on elbait
constexpr size_t blocks_per_page = 4;
constexpr size_t cache_pages = 8;
constexpr size_t block_size = 16 * 4096 * sizeof(uint_t<40>);

using filemapped_vector_t = stxxl::VECTOR_GENERATOR<
    uint_t<40>, blocks_per_page, cache_pages, block_size>::result;

template<class text_t>
void diskstrategy(text_t& text, size_t threshold, lzss::FactorBufferDisk& refs) {
    DVLOG(2) << "diskstrategy for text of size " << text.size();
    DCHECK_EQ(0, text[text.size()-1]);

	text.require(ds::SA | ds::ISA);

    char* safile { new char[TMP_MAX] };
    char* isafile { new char[TMP_MAX] };
    char* plcpfile { new char[TMP_MAX] };

    std::tmpnam(safile);
    std::tmpnam(isafile);
    std::tmpnam(plcpfile);

    {
        DVLOG(2) << "compute SA";
        const auto& sa = text.require_sa();
        std::ofstream sao(safile);
        for(auto it = sa.begin(); it != sa.end(); ++it) {
            uint_t<40> buf { *it };
            static_assert(sizeof(buf) == 5);
            sao.write(reinterpret_cast<const char*>(&buf), sizeof(buf));
        }
    }
    {
        DVLOG(2) << "compute ISA";
        const auto& isa = text.require_isa();
        std::ofstream isao(isafile);
        for(auto it = isa.begin(); it != isa.end(); ++it) {
            uint_t<40> buf { *it };
            static_assert(sizeof(buf) == 5);
            isao.write(reinterpret_cast<const char*>(&buf), sizeof(buf));
        }
    }
    {
        DVLOG(2) << "compute PLCP";
        std::ofstream plcpo(plcpfile);
   		construct_plcp_bitvector(text.require_sa(), text).write_data(plcpo);
    }
    {
        DVLOG(2) << "map STXXL vectors";
        stxxl::syscall_file f_sa(safile, stxxl::file::open_mode::RDONLY);
        filemapped_vector_t sa(&f_sa);

        stxxl::syscall_file f_isa(isafile, stxxl::file::open_mode::RDONLY);
        filemapped_vector_t isa(&f_isa);

        // check index ds
        {
            DVLOG(2) << "check index ds";
            PLCPFileForwardIterator pplcp    (plcpfile);
            const auto& tsa = text.require_sa();
            const auto& tisa = text.require_isa();
            const auto& plcp = text.require_plcp();

            for(size_t i = 0; i < sa.size(); ++i) {
                DCHECK_EQ(sa.size(),tsa.size());
                DCHECK_EQ(sa[i], (uint64_t)tsa[i]);
            }

            DCHECK_EQ(isa.size(),tisa.size());

            for(size_t i = 0; i < isa.size(); ++i) {
                DCHECK_EQ(isa[i], (uint64_t)tisa[i]);
            }

            for(size_t i = 0; i < plcp.size()-1; ++i) {
                DCHECK_EQ(pplcp(),(uint64_t) plcp[i]);
                pplcp.advance();
            }
        }

        {
            DVLOG(2) << "factorize";
     		PLCPFileForwardIterator pplcp(plcpfile);
	        RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
    	    compute_references(text.size(), refStrategy, pplcp, threshold);
	        refStrategy.factorize(refs);
        }
    }

    DVLOG(2) << "clean up";
    delete [] safile;
    delete [] isafile;
    delete [] plcpfile;

    DVLOG(2) << "done (refs: " << refs.size() << ")";
}

template<class text_t>
void test_plcp(text_t& text) {
    using namespace tdc::lcpcomp;

    lzss::FactorBufferRAM bufRAM;
    {
        PLCPStrategy strat(create_env(PLCPStrategy::meta()));
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
        auto coder = test_coder_t(tdc::create_env(test_coder_t::meta())).encoder(
            outputRAM, tdc::lzss::UnreplacedLiterals<text_t,decltype(bufRAM)>(text, bufRAM));

        coder.encode_text(text, bufRAM);
    }
    {
        tdc::Output outputDisk(outbufDisk);
        auto coder = test_coder_t(tdc::create_env(test_coder_t::meta())).encoder(
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
		textds_t t = create_algo<textds_t>("", in);
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
	test::on_string_generators(runner,13);
TEST(plcp, filecheck)          { TEST_DS_STRINGCOLLECTION(test_plcp); }
#undef TEST_DS_STRINGCOLLECTION

