#ifndef TUDOCOMP_ALGORITHM_H
#define TUDOCOMP_ALGORITHM_H

#include <vector>
#include <string>

#include "tudocomp.h"
#include "global_registry.h"

namespace tudocomp_driver {

void register_algos(AlgorithmRegistry<Compressor>& registry);

}

#endif
