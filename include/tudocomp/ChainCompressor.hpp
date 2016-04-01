#ifndef _INCLUDED_CHAIN_COMPRESSOR_HPP_
#define _INCLUDED_CHAIN_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <vector>
#include <memory>

namespace tudocomp {

class ChainCompressor: public Compressor {
private:
    std::vector<CompressorConstructor> m_compressors;

public:
    /// No default construction allowed
    inline ChainCompressor() = delete;

    /// Construct the class with an environment and the algorithms to chain.
    inline ChainCompressor(Env& env,
                           std::vector<CompressorConstructor>&& compressors):
                           Compressor(env),
                           m_compressors(std::move(compressors)) {}

    template<class F>
    inline void chain(Input& input, Output& output, bool reverse, F f) {
        std::vector<uint8_t> in_buf;
        std::vector<uint8_t> out_buf;
        for (size_t _i = 0; _i < m_compressors.size(); _i++) {
            size_t first = 0;
            size_t last = m_compressors.size() - 1;
            size_t i = _i;
            if (reverse) {
                i = last - i;
                std::swap(first, last);
            }
            {
                // first, choose whether to put input/output into a buffer or not

                Input buffer_input = Input::from_memory(in_buf);
                Input* chain_input;

                // first algorithm?
                if (i == first) {
                    chain_input = &input;
                } else {
                    chain_input = &buffer_input;
                }

                out_buf.clear();
                Output buffer_output = Output::from_memory(out_buf);
                Output* chain_output;

                // last algorithm?
                if (i == last) {
                    chain_output = &output;
                } else {
                    chain_output = &buffer_output;
                }

                // then invoke the algorithm
                auto compressor = m_compressors[i](*m_env);
                f(*chain_input, *chain_output, *compressor);
            }

            std::swap(in_buf, out_buf);
        }
    }

    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void compress(Input& input, Output& output) override final {
        std::cout << "compress\n";
        chain(input, output, false, [](Input& i, Output& o, Compressor& c) {
            c.compress(i, o);
        });
        std::cout << "compress done\n";
    }

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    inline virtual void decompress(Input& input, Output& output) override final {
        std::cout << "decompress\n";
        chain(input, output, true, [](Input& i, Output& o, Compressor& c) {
            c.decompress(i, o);
        });
        std::cout << "decompress done\n";
    }
};

}

#endif
