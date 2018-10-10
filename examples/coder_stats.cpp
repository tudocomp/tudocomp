// Computes for each number the number of bits needed to represent the respective number in a particular coding.
//
#include <glog/logging.h>

#include <fstream>
#include <functional>
#include <tudocomp/Range.hpp>

#include <tudocomp/util/bits.hpp>
#include <tudocomp/util/int_coder.hpp>

// #include "test/util.hpp"

using namespace tdc;



class AccumSink { //! accumulates the number of bits needed to represent a number
	public: 
	void write_bit(bool) {
		++m_bits;
	}
	void write_int(uint64_t, uint8_t bits) {
		m_bits += bits; //tdc::bits_for(i);
	}

	size_t m_bits = 0; // counts number of bits
};

void write_integers(const std::string& filename, std::function<void(AccumSink&,uint64_t)> funct) {
	LengthRange range;
	AccumSink sink;
	std::ofstream o(filename, std::ios::out);
	for(size_t i = 0; i < 2<<16; ++i) {
		funct(sink,i);
		o << i << "\t" << sink.m_bits << std::endl;
		sink.m_bits = 0;
	}
}


int main(int argc, const char** argv) {
	write_integers("stat_unary.txt", [] (AccumSink& s,uint64_t i) { tdc::write_unary(s,i); } );
	write_integers("stat_ternary.txt", [] (AccumSink& s,uint64_t i) { tdc::write_ternary(s,i); });
	write_integers("stat_int2.txt", [] (AccumSink& s,uint64_t i) { tdc::write_compressed_int(s,i,2); } );
	write_integers("stat_int4.txt", [] (AccumSink& s,uint64_t i) { tdc::write_compressed_int(s,i,4); } );
	write_integers("stat_int6.txt", [] (AccumSink& s,uint64_t i) { tdc::write_compressed_int(s,i,6); } );
	write_integers("stat_int8.txt", [] (AccumSink& s,uint64_t i) { tdc::write_compressed_int(s,i,8); } );
	write_integers("stat_elias_gamma.txt", [] (AccumSink& s,uint64_t i) { tdc::write_elias_gamma(s,i); } );
	write_integers("stat_elias_delta.txt", [] (AccumSink& s,uint64_t i) {  tdc::write_elias_delta(s,i);} );
	write_integers("stat_rice2.txt", [] (AccumSink& s,uint64_t i) { tdc::write_rice(s,i,2); } );
	write_integers("stat_rice4.txt", [] (AccumSink& s,uint64_t i) { tdc::write_rice(s,i,4); } );
	write_integers("stat_rice6.txt", [] (AccumSink& s,uint64_t i) { tdc::write_rice(s,i,6); } );
	write_integers("stat_rice8.txt", [] (AccumSink& s,uint64_t i) { tdc::write_rice(s,i,8); } );
	write_integers("stat_binary.txt", [] (AccumSink& s,uint64_t i) { s.write_int(0, tdc::bits_for(i)); } );

}
