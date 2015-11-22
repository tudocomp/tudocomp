#ifndef COMPRESSFRAMEWORK_H
#define COMPRESSFRAMEWORK_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <istream>
#include <streambuf>
#include <sstream>
#include <string>
#include <map>
#include <unordered_map>

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"

#include "tudocomp_env.h"
#include "tudocomp_util.h"

namespace tudocomp {

/// Type of the input data to be compressed
using Input = std::vector<uint8_t>;

/// Interface for a general compressor.
struct Compressor {
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Compressor() = delete;

    /// Construct the class with an environment.
    inline Compressor(Env& env_): env(env_) {}

    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void compress(Input input, std::ostream& out) = 0;

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(std::istream& inp, std::ostream& out) = 0;
};

}

#endif
