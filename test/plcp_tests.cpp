#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include "test/util.hpp"
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>

using namespace tdc;
using namespace tdc::lcpcomp;
using namespace tdc::lzss;
using io::InputView;



template<class text_t>
void diskstrategy(text_t& text, size_t threshold, lzss::FactorBufferDisk& refs) {
		StatPhase phase("Load Index DS");
		text.require(text_t::SA | text_t::ISA);

		//RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);

        char* safile { new char[TMP_MAX] };
        char* isafile { new char[TMP_MAX] };
        char* plcpfile { new char[TMP_MAX] };
        std::tmpnam(safile);
        std::tmpnam(isafile);
        std::tmpnam(plcpfile);
        {
            const auto& sa = text.require_sa();
            std::ofstream sao(safile);
            for(auto it = sa.begin(); it != sa.end(); ++it) {
                uint_t<40> buf { *it };
                static_assert(sizeof(buf) == 5);
                sao.write(reinterpret_cast<const char*>(&buf), sizeof(buf));
            }
        }
        {
            const auto& isa = text.require_isa();
            std::ofstream isao(isafile);
            for(auto it = isa.begin(); it != isa.end(); ++it) {
                uint_t<40> buf { *it };
                static_assert(sizeof(buf) == 5);
                isao.write(reinterpret_cast<const char*>(&buf), sizeof(buf));
            }
        }
        {
            std::ofstream plcpo(plcpfile);
       		construct_plcp_bitvector(text.require_sa(), text).write_data(plcpo);
        }

        IntegerFileArray<uint_t<40> > sa  (safile);
		IntegerFileArray<uint_t<40> > isa (isafile);


        StatPhase::wrap("Check Index DS", [&]{
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
                });

 		PLCPFileForwardIterator pplcp    (plcpfile);

		RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
		StatPhase::wrap("Search Peaks", [&]{
				compute_references(text.size(), refStrategy, pplcp, threshold);
		});
		StatPhase::wrap("Compute References", [&]{
				refStrategy.factorize(refs);
				});

        delete [] safile;
        delete [] isafile;
        delete [] plcpfile;
    }

template<class text_t>
void test_plcp(text_t& text) {
    using namespace tdc::lcpcomp;
    PLCPStrategy strat(create_env(Meta("plcp","Hi")));
    lzss::FactorBufferRAM bufRAM;
    strat.factorize(text, 5, bufRAM);
    lzss::FactorBufferDisk bufDisk;
    diskstrategy(text, 5, bufDisk);
    ASSERT_EQ(bufRAM.size(), bufDisk.size());
    {
        auto itRAM = bufRAM.begin();
        auto itDisk = bufDisk.begin();
        while(itRAM != bufRAM.end() && itDisk != bufDisk.end()) {
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
        tdc::Env env = tdc::create_env(Meta("plcpcomp", "plcp"));
        tdc::BitCoder::Encoder coder(std::move(env), outputRAM, tdc::lzss::TextLiterals<text_t,decltype(bufRAM)>(text, bufRAM));
        tdc::lzss::encode_text(coder, text, bufRAM);
    }
    {
        tdc::Output outputDisk(outbufDisk);
        tdc::Env env = tdc::create_env(Meta("plcpcomp", "plcp"));
        tdc::BitCoder::Encoder coder(std::move(env), outputDisk, tdc::lzss::TextLiterals<text_t,decltype(bufDisk)>(text, bufDisk));
        tdc::lzss::encode_text(coder, text, bufDisk);
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
		VLOG(2) << "str = \"" << str << "\"" << " size: " << str.length();
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

