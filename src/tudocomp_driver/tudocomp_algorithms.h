#ifndef TUDOCOMP_ALGORITHM_H
#define TUDOCOMP_ALGORITHM_H

#include <vector>
#include <string>

#include "tudocomp.h"

namespace tudocomp_driver {

template<class T>
using Constructor = T* (*)(tudocomp::Env&);

template<class Base, class T, class ... Args>
Base* construct(Args ... args) {
    return new T(args...);
}

/// Struct for holding all necessary data to register a compression algorithm
struct CompressionAlgorithm {
    /// Human readable name
    std::string name;
    /// Used as id string for command line and output filenames
    std::string shortname;
    /// Description text
    std::string description;
    /// Algorithm
    Constructor<tudocomp::Compressor> compressor;
};
extern std::vector<CompressionAlgorithm> COMPRESSION_ALGORITHM;

inline CompressionAlgorithm getCompressionByShortname(std::string s) {
    for (auto& x: COMPRESSION_ALGORITHM) {
        if (x.shortname == s) {
            return x;
        }
    }
    return { "", s, "", nullptr };
}

/// Struct for holding all necessary data to register a encoding algorithm
struct CodingAlgorithm {
    /// Human readable name
    std::string name;
    /// Used as id string for command line and output filenames
    std::string shortname;
    /// Description text
    std::string description;
    /// Algorithm
    Constructor<tudocomp::Coder> coder;
};
extern std::vector<CodingAlgorithm> CODING_ALGORITHM;

inline CodingAlgorithm getCodingByShortname(std::string s) {
    for (auto& x: CODING_ALGORITHM) {
        if (x.shortname == s) {
            return x;
        }
    }
    return { "", s, "", nullptr };
}

}

#endif
