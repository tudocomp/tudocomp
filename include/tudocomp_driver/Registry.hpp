#ifndef TUDOCOMP_ALGORITHMS_REGISTRY_H
#define TUDOCOMP_ALGORITHMS_REGISTRY_H

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <memory>

#include <glog/logging.h>

#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/AlgorithmStringParser.hpp>
#include <tudocomp/Registry.hpp>

/// \brief Contains the executable driver application.
///
/// The driver application is a standalone executable that makes the
/// framework's compression and encoding algorithms available for use in a
/// command-line utility.
///
/// For algorithms to be made available in the driver application, they need
/// to be registered in the \ref Registry. Any registered algorithm will also
/// be listed in the utility's help message.
namespace tdc_driver {}

namespace tdc_algorithms {

using namespace tdc;

/// \cond INTERNAL
extern Registry REGISTRY;
/// \endcond

/// \brief Called when the driver application compiles its list of available
///        algorithms.
///
/// \param registry The application's registry.
void register_algorithms(Registry& registry);

}

#endif
