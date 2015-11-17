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

}

#endif
