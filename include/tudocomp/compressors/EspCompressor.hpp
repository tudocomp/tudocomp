#ifndef _INCLUDED_ESP_COMPRESSOR_HPP
#define _INCLUDED_ESP_COMPRESSOR_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>

namespace tdc {

class EspCompressor: public Compressor {

// Implementation that covers all of 64 bit
// TODO: Does the Paper mean base-e or base-2 ?
inline size_t iter_log(size_t n) {
    if (n < 3) return 1;
    if (n < 16) return 2;
    if (n < 3814280) return 3;
    return 4;
}

struct MetaBlock {
    size_t type;
    View view;
};

void meta_blocks_debug(const std::vector<MetaBlock> meta_blocks, View in) {
    {
        std::stringstream ss;
        for (auto& mb : meta_blocks) {
            ss << mb.view;
        }
        CHECK(ss.str() == std::string(in));
    }

    std::cout << "|";
    for (auto& mb : meta_blocks) {
        size_t w = mb.view.size() / 2;
        std::cout << std::setw(w) << "";
        std::cout << std::left << std::setw(mb.view.size() - w) << mb.type;
        std::cout << "|";
    }
    std::cout << "\n";

    std::cout << "|";
    for (auto& mb : meta_blocks) {
        std::cout << mb.view << "|";
    }
    std::cout << "\n";
}

template<class T>
uint64_t calc_alphabet_size(const T& t) {
    Counter<typename T::value_type> c;
    for (auto v : t) {
        c.increase(v);
    }
    return c.getNumItems();
}

template<class F>
inline void handle_meta_block_2(View A,
                                uint64_t alphabet_size,
                                std::vector<uint8_t>& buf,
                                F debug_push_meta_block) {
    buf.clear();
    buf.insert(buf.cbegin(), A.cbegin(), A.cend());

    auto calc_label = [](uint64_t left, uint64_t right) {
        auto diff = left ^ right;

        DCHECK(diff != 0);

        auto l = __builtin_ctz(diff);

        auto bit = [](uint8_t l, uint64_t v) {
            // TODO: test
            return (v >> l) & 1;
        };

        // form label(A[i])
        return 2*l + bit(l, right);
    };

    std::cout << vec_to_debug_string(buf) << "\n";

    size_t start = 0;

    for (uint shrink_i = 0; shrink_i < iter_log(alphabet_size); shrink_i++) {
        for (size_t i = start + 1; i < buf.size(); i++) {
            auto left  = buf[i - 1];
            auto right = buf[i];
            buf[i] = calc_label(left, right);
        }

        // TODO: This is for debuging - no label for this location
        buf[start] = -1;

        start++;

        std::cout << vec_to_debug_string(buf) << "\n";
    }

    A = View(buf).substr(start);
    std::cout << vec_to_debug_string(A) << "\n";

    DCHECK(calc_alphabet_size(A) <= 6);

    // TODO: This would benefit from a general, mutable, slice type

    // final pass: reduce to alphabet 3
    for(uint to_replace = 3; to_replace < 6; to_replace++) {
        for (size_t i = start; i < buf.size(); i++) {
            if (buf[i] == to_replace) {
                uint64_t neighbors[2];
                uint8_t neighbor_len = 0;

                if (i == start && i == buf.size() - 1) {
                    neighbor_len = 0;
                } else if (i == start) {
                    neighbor_len = 1;
                    neighbors[0] = buf[i + 1];
                } else if (i == buf.size() - 1) {
                    neighbor_len = 1;
                    neighbors[0] = buf[i - 1];
                } else {
                    neighbor_len = 2;
                    neighbors[0] = buf[i - 1];
                    neighbors[1] = buf[i + 1];
                }

                buf[i] = 0;
                for (uint8_t j = 0; j < neighbor_len; j++) {
                    if (neighbors[j] == buf[i]) {
                        buf[i]++;
                    }
                }
                for (uint8_t j = 0; j < neighbor_len; j++) {
                    if (neighbors[j] == buf[i]) {
                        buf[i]++;
                    }
                }
            }
        }
        std::cout << vec_to_debug_string(A) << "\n";
    }
}

public:
    inline static Meta meta() {
        Meta m("compressor", "esp", "ESP based grammar compression");
        //m.option("coder").templated<coder_t>();
        //m.option("min_run").dynamic("3");
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();

        size_t alphabet_size = 256;

        std::vector<MetaBlock> meta_blocks;

        {
            auto push_meta_block = [&](size_t type, size_t from, size_t to) {
                meta_blocks.push_back(MetaBlock { type, in.substr(from, to) });
            };

            size_t i = 0;

            while(i < in.size()) {
                auto type_1_start = i;
                while ((i < (in.size() - 1)) && (in[i] == in[i + 1])) {
                    i++;
                }
                if ((i - type_1_start) > 0) {
                    push_meta_block(1, type_1_start, i + 1);
                    i++;
                }

                auto type_23_start = i;
                while (i < (in.size() - 1) && in[i] != in[i + 1]) {
                    i++;
                }

                if (i == in.size() - 1) {
                    i++;
                }
                size_t type_23_len = i - type_23_start;
                if (type_23_len > 0) {
                    if (type_23_len >= iter_log(alphabet_size)) {
                        push_meta_block(2, type_23_start, i);
                    } else {
                        push_meta_block(3, type_23_start, i);
                    }
                }
            }

            meta_blocks_debug(meta_blocks, in);
        }

        std::vector<View> blocks;

        std::vector<uint8_t> buf;

        for (auto& meta_block : meta_blocks) {
            View A = meta_block.view;

            if (meta_block.type == 2) {
                handle_meta_block_2(A, alphabet_size, buf, [](){});
            } else {

            }

            std::cout << "---\n";
        }


    }

    inline virtual void decompress(Input& input, Output& output) override {

    }
};

}

#endif
