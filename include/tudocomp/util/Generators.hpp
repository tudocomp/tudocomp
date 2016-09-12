#ifndef GENERATORS_HPP
#define GENERATORS_HPP

#include <random>
#include <string>

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
std::string fibonacci_word(size_t n) {
	if( n == 1) return "b";
	if( n == 2) return "a";
	return fibonacci_word(n-1) + fibonacci_word(n-2);
}

#endif /* GENERATORS_HPP */
