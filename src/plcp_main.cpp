#include <algorithm>
#include <iostream>
#include <tudocomp_stat/StatPhase.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>

namespace tdc { namespace lcpcomp {
    inline void factorize(const std::string& textfilename, size_t threshold, lzss::FactorBuffer& refs) {
		StatPhase phase("PLCPComp");
		IntegerFileArray<uint_t<40>> sa  ((textfilename + ".sa5").c_str());
		IntegerFileArray<uint_t<40>> isa ((textfilename + ".isa5").c_str());
		// SAFileArray<uint_t<40>> sa((textfilename + ".sa5").c_str());
		// ISAFileArray<uint_t<40>> isa((textfilename + ".isa5").c_str());
		//IntegerFileForwardIterator<uint_t<40>> pplcp("/bighome/workspace/compreSuite/tudocomp/datasets/pc_english.200MB.plcp5");

// IF_DEBUG(
// 		DCHECK_EQ(sa.size(), text.size());
// 		StatPhase::wrap("Check Index DS", [&]{
// 			const auto& tsa = text.require_sa();
// 			const auto& tisa = text.require_isa();
// 			const auto& plcp = text.require_plcp();
// 				PLCPFileForwardIterator pplcp    ((textfilename + ".plcp").c_str());
// 			for(size_t i = 0; i < sa.size(); ++i) {
// 			DCHECK_EQ(sa.size(),tsa.size());
// 			DCHECK_EQ(sa[i], (uint64_t)tsa[i]);
// 			}
// 			DCHECK_EQ(isa.size(),tisa.size());
// 			for(size_t i = 0; i < isa.size(); ++i) {
// 			DCHECK_EQ(isa[i], (uint64_t)tisa[i]);
// 			}
// 			for(size_t i = 0; i < plcp.size()-1; ++i) {
// 			DCHECK_EQ(pplcp(),(uint64_t) plcp[i]);
// 			pplcp.advance();
// 			}
// 		});
// 		);

		PLCPFileForwardIterator pplcp    ((textfilename + ".plcp").c_str());

		RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
		StatPhase::wrap("Search Peaks", [&]{
				compute_references(filesize(textfilename.c_str())-1, refStrategy, pplcp, threshold);
		});
		StatPhase::wrap("Compute References", [&]{
				refStrategy.factorize(refs);
				});

	}
}}//ns


bool file_exist(const std::string& filepath) {
	std::ifstream file(filepath);
	return !file.fail();
}


int main(int argc, char** argv) {
	if(argc <= 1) {
		std::cerr << "Usage : " << argv[0] << " [infile] [outfile] " << std::endl;
		return 1;
	}
	tdc::StatPhase root("Root");

	const std::string infile { argv[1] };
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

	tdc::lzss::FactorBuffer buf;
	tdc::lcpcomp::factorize(infile, 5, buf);

	root.to_json().str(std::cout);
	std::cout << std::endl;

    return 0;
}

