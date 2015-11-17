#ifndef TUDOCOMP_ALGORITHM_H
#define TUDOCOMP_ALGORITHM_H

#include <vector>
#include <string>

#include "tudocomp.h"

namespace tudocomp_driver {

using tudocomp::Constructor;
using tudocomp::Algorithm;
using tudocomp::Lz77RuleCompressor;
using tudocomp::Lz77RuleCoder;

using CompressionAlgorithm = Algorithm<Lz77RuleCompressor>;
using CodingAlgorithm = Algorithm<Lz77RuleCoder>;

inline CompressionAlgorithm getCompressionByShortname(std::string s) {
    for (auto& x: tudocomp::LZ77_RULE_COMP_ALGOS.registry) {
        if (x->shortname == s) {
            return *x;
        }
    }
    return { "", s, "", nullptr };
}

inline CodingAlgorithm getCodingByShortname(std::string s) {
    for (auto& x: tudocomp::LZ77_RULE_CODE_ALGOS.registry) {
        if (x->shortname == s) {
            return *x;
        }
    }
    return { "", s, "", nullptr };
}

}

#endif
