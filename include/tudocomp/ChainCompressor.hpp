#ifndef _INCLUDED_CHAIN_COMPRESSOR_HPP_
#define _INCLUDED_CHAIN_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <vector>
#include <memory>

namespace tdc {

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

            auto av = option_value.as_algorithm();

            bool needs_sentinel = av.needs_sentinel_terminator();

            auto compressor = create_algo_with_registry_dynamic(
                env().registry(), av);

            f(i, o, *compressor, needs_sentinel);
        };

        std::vector<uint8_t> between_buf;
        {
            Output between(between_buf);
            run(input, between, first_algo);
        }
        DLOG(INFO) << "Buffer between chain: " << vec_to_debug_string(between_buf);
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
        chain(input, output, false, [](Input& i, Output& o, Compressor& c, bool needs_sentinel) {
            if (needs_sentinel) {
                i.escape_and_terminate();
            }
            c.compress(i, o);
        });
    }

    /// Decompress `inp` into `out`.
    ///
    /// \param input The input stream.
    /// \param output The output stream.
    inline virtual void decompress(Input& input, Output& output) override final {
        chain(input, output, true, [](Input& i, Output& o, Compressor& c, bool needs_sentinel) {
            if (needs_sentinel) {
                o.unescape_and_trim();
            }
            c.decompress(i, o);
        });
    }
};

}

#endif
