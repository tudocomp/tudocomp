#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/pre_header.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

namespace tdc {
namespace esp {
    struct TreeReducer {
        using in_t = ConstGenericView<size_t>;

        void reduce(string_ref input) {
            std::vector<size_t> layer;
            layer.reserve(input.size());
            for (auto c : input) {
                layer.push_back(c);
            }

            size_t alphabet = 256;

            std::vector<size_t> new_layer;

            in_t in = layer;

            {
                    esp::Context<in_t> ctx {
                        alphabet,
                        in,
                    };
                    ctx.print_mb2_trace = false;

                    std::cout << "             " << debug_p(in, alphabet) << "\n";
                    esp::split(in, ctx);
                    std::cout << "\n[Adjusted]:\n\n";
                    ctx.adjusted_blocks();
            }

        }
    };


}
}
