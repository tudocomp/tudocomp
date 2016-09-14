#ifndef _INCLUDED_CHAIN_COMPRESSOR_HPP_
#define _INCLUDED_CHAIN_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/CreateAlgorithm.hpp>
#include <vector>
#include <memory>

namespace tudocomp {

class ChainCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "chain");
        m.option("first").dynamic_compressor();
        m.option("second").dynamic_compressor();
        return m;
    }

    /// No default construction allowed
    inline ChainCompressor() = delete;

    /// Construct the class with an environment and the algorithms to chain.
    inline ChainCompressor(Env&& env):
        Compressor(std::move(env)) {}

    template<class F>
    inline void chain(Input& input, Output& output, bool reverse, F f) {
        string_ref first_algo = "first";
        string_ref second_algo = "second";
        if (reverse) {
            std::swap(first_algo, second_algo);
        }

        auto run = [&](Input& i, Output& o, string_ref option) {
            auto& option_value = env().option(option);
            DCHECK(option_value.is_algorithm());
            auto compressor = create_algo_with_registry_dynamic(
                env().registry(), option_value.as_algorithm());

            f(i, o, *compressor);
        };

        std::vector<uint8_t> between_buf;
        {
            Output between(between_buf);
            run(input, between, first_algo);
        }
        {
            Input between(between_buf);
            run(between, output, second_algo);
        }
    }

    /// Compress `inp` into `out`.
    ///
    /// \param input The input stream.
    /// \param output The output stream.
    inline virtual void compress(Input& input, Output& output) override final {
        std::cout << "compress\n";
        chain(input, output, false, [](Input& i, Output& o, Compressor& c) {
            c.compress(i, o);
        });
        std::cout << "compress done\n";
    }

    /// Decompress `inp` into `out`.
    ///
    /// \param input The input stream.
    /// \param output The output stream.
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
