#pragma once

#include <random>
#include <string>
#include <cmath>

namespace Ranges {
	std::pair<size_t,size_t> numbers(48,57);
	std::pair<size_t,size_t> printable(33,123);
};

std::string random_uniform(const size_t length, const std::pair<size_t,size_t> range = Ranges::numbers, size_t seed = 0) {
	std::string s(length,0);
	std::default_random_engine engine(seed);
	std::uniform_int_distribution<char> dist(range.first,range.second);
	for(size_t i = 0; i < length; ++i) {
		s[i] = dist(engine);
	}
	return s;
}
/** outputs the n-th fibonacci word.
 * For n=45 the string already has more than 1 mio. characters,
 * i.e., takes more than 1GB.
 */
std::string fibonacci_word(size_t n) {
	if( n == 1) return "b";
	if( n == 2) return "a";
	std::string vold = "b";
	std::string old = "a";
	vold.reserve(std::pow(1.62,n-1));
	old.reserve(std::pow(1.62,n));
	for(size_t i = 2; i < n; ++i) {
		std::string tmp = old + vold;
		vold = old;
		old = tmp;
	}
	DCHECK_LE(old.length(), std::pow(1.62,n));
	DCHECK_LE(vold.length(), std::pow(1.62,n-1));
	return old;
}

/** outputs the n-th Thue Morse word
 * For n=32 the string already has more than 2 mio. characters,
 * i.e., takes more than 2GB.
 */
std::string thue_morse_word(size_t n) {
	if(n == 0) return "0";
	std::string a = "0";
	a.reserve(1ULL<<n);
	for(size_t i = 1; i < n; ++i) {
		const size_t len = a.length();
		for(size_t j = 0; j < len; ++j) {
			a.push_back(a[j] == '0' ? '1' : '0');
		}

	}
	DCHECK_LE(a.length(), 1ULL<<n);
	return a;
}

/** From: A Series of Run-Rich Strings
 * Wataru Matsubara et al.
 */
std::string run_rich(size_t n) {
	std::string t0 = "0110101101001011010",
		t1 = "0110101101001",
		t2 = "01101011010010110101101",
		t3 = t2+t1;
	if(n==0) return t0;
	if(n==1) return t1;
	if(n==2) return t2;

	for(size_t i = 4; i < n; ++i) {
		std::string tmp = (i % 3 == 0) ? (t3+t2) : (t3+t0);
		t0=t1;
		t1=t2;
		t2=t3;
		t3=tmp;
	}
	return t3;
}

