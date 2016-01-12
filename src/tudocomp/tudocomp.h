#ifndef COMPRESSFRAMEWORK_H
#define COMPRESSFRAMEWORK_H

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "boost/utility/string_ref.hpp"
#include "boost/variant.hpp"
#include "boost/iostreams/stream_buffer.hpp"
#include "boost/iostreams/device/back_inserter.hpp"
#include "glog/logging.h"

#include "tudocomp_env.h"
#include "tudocomp_util.h"
#include "tudocomp_io_handle.h"

namespace tudocomp {

/// Handle of the input data stream.
using Input = input::Input;

/// Handle of the output data stream.
using Output = output::Output;

/// Interface for a general compressor.
struct Compressor {
    // TODO: Maybe replace by pointer to make this class properly copyable?
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Compressor() = delete;

    /// Construct the class with an environment.
    inline Compressor(Env& env_): env(env_) {}

    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void compress(Input& input, Output& output) = 0;

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(Input& input, Output& output) = 0;
};

}

#endif
