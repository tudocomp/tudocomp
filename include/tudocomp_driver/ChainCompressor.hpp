#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Registry.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp_driver/Registry.hpp>
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

            auto textds_flags = av.textds_flags();

            DVLOG(1) << "dynamic creation of" << av.name() << "\n";
            auto compressor = create_algo_with_registry_dynamic(
                tdc_algorithms::COMPRESSOR_REGISTRY, av);

            f(i, o, *compressor, textds_flags);
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
        chain(input, output, false, [](Input& i,
                                       Output& o,
                                       Compressor& c,
                                       ds::InputRestrictionsAndFlags flags) {
            bool res = flags.has_restrictions();
            if (res) {
                auto i2 = Input(i, flags);
                c.compress(i2, o);
            } else {
                c.compress(i, o);
            }
        });
    }

    /// Decompress `inp` into `out`.
    ///
    /// \param input The input stream.
    /// \param output The output stream.
    inline virtual void decompress(Input& input, Output& output) override final {
        chain(input, output, true, [](Input& i,
                                      Output& o,
                                      Compressor& c,
                                      ds::InputRestrictionsAndFlags flags) {
            bool res = flags.has_restrictions();
            if (res) {
                auto o2 = Output(o, flags);
                c.decompress(i, o2);
            } else {
                c.decompress(i, o);
            }
        });
    }
};

}

