#ifndef _INCLUDED_ALGORITHM_HEADER_COMPRESSOR_HPP_
#define _INCLUDED_ALGORITHM_HEADER_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <vector>
#include <memory>

namespace tudocomp {

/// A Compressor adapter that on compression just prefixes
/// the input with `m_algorithm_header` before outputting it,
/// and that on decompression stores such a header in
/// `m_algorithm_header`.
///
/// The format of the header looks like this:
///
///     <contents of m_algorithm_header>%
///
/// That is, % is used as an escape symbol to mark end-of-header.
class AlgorithmHeaderCompressor: public Compressor {
private:
    std::string m_algorithm_header;

public:
    /// No default construction allowed
    inline AlgorithmHeaderCompressor() = delete;

    /// Construct the class with an environment.
    inline AlgorithmHeaderCompressor(Env& env): Compressor(env) {}

    inline void set_algorithm_header(std::string s) {
        CHECK(s.find('%') == string::npos);
        m_algorithm_header = s;
    }

    inline std::string algorithm_header() {
        return m_algorithm_header;
    }

    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) override final {
        auto i_stream = input.as_stream();
        auto o_stream = output.as_stream();
        (*o_stream) << m_algorithm_header << '%' << (*i_stream);
    }

    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void decompress(Input& input, Output& output) override final {

    }
};

}

#endif
