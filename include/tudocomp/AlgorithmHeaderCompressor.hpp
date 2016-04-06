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

    using DecompressSelector
        = std::function<std::unique_ptr<Compressor> (std::string)>;

    DecompressSelector m_decompress_selector;

public:
    /// No default construction allowed
    inline AlgorithmHeaderCompressor() = delete;

    /// Construct the class with an environment.
    inline AlgorithmHeaderCompressor(Env& env,
                                     std::string algorithm_header,
                                     DecompressSelector selector):
        Compressor(env), m_algorithm_header(algorithm_header),
        m_decompress_selector(selector) {
        CHECK(m_algorithm_header.find('%') == string::npos);
    }

    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) override final {
        {
            auto o_stream = output.as_stream();
            (*o_stream) << m_algorithm_header << '%';
        }
        m_compressor.compress(input, output);
    }

    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void decompress(Input& input, Output& output) override final {
        {
            auto i_stream = input.as_stream();
            std::string algorithm_header;

            char c;
            size_t sanity_size_check = 0;
            while ((*i_stream),get(c)) {
                if (sanity_size_check > 1023) {
                    m_env.error("Input did not start with an Algorithm header");
                } else if (c == '%') {
                    break;
                } else {
                    algorithm_header.push_back(c);
                }
                sanity_size_check++;
            }
            m_algorithm_header = std::move(algorithm_header);
        }

        m_compressor.decompress(input, output)
    }
};

}

#endif
