#include <algorithm>
#include <iostream>
#include <tudocomp_stat/StatPhase.hpp>



int main(int argc, char** argv) {
	if(argc <= 2) {
		std::cerr << "Usage : " << argv[0] << " [infile] [outfile] " << std::endl;
		return 1;
	}
	tdc::StatPhase root("Root");

	const char* infile = argv[0];

	root.to_json().str(std::cout);

    return 0;
}

