#pragma once

#include <tudocomp/compressors/esp/TypedBlock.hpp>
#include <tudocomp/compressors/esp/BlockAdjust.hpp>
#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    template<typename Source>
    struct Context {
        size_t alphabet_size;
        std::vector<size_t> scratchpad;
        Source s;
        Source sb;
        size_t i = 0;
        size_t last_i = 0;
        bool print_mb_trace = true;
        bool print_mb2_trace = true;
        bool print_only_adjusted = false;
        bool print_bulk_text = false;

        bool behavior_landmarks_tie_to_right = true;
        bool behavior_metablocks_maximimze_repeating = true;

        std::vector<TypedBlock> block_buffer;

        Context(size_t as, Source src):
            alphabet_size(as),
            scratchpad(),
            s(src),
            sb(src),
            i(0),
            last_i(0)
        {}

        void reset_debug_print() {
            sb = s;
        }

        void check_sizes(string_ref errmsg) {
            size_t full_size = 0;
            for (auto& b : block_buffer) {
                full_size += b.len;
            }
            DCHECK_EQ(full_size, s.size()) << errmsg;
        }

        void print_cut(size_t l, size_t type, bool doit) {
            auto front_cut = sb.substr(0, l);
            auto back_cut = sb.substr(l);

            auto n = s.size() - (back_cut.size() + front_cut.size());
            n *= debug_p(sb, alphabet_size).char_mult();

            IF_DEBUG(if (doit && print_mb_trace) {
                std::cout << "mblock " << type << ": ";
                if (print_bulk_text) std::cout << std::setw(n) << "";
                std::cout << debug_p(front_cut, alphabet_size);
                if (print_bulk_text) std::cout << " ";
                if (print_bulk_text) std::cout << debug_p(back_cut, alphabet_size);
                std::cout << "\n";
                if (l < 2 || l > 3) std::cout << "Needs adjustment!\n";
            })

            sb = back_cut;
        }

        void print_all(bool doit) {
            reset_debug_print();
            for (auto& b : block_buffer) {
                print_cut(b.len, b.type, doit);
            }
        }

        void push_back(size_t l, size_t type) {
            print_cut(l, type, !print_only_adjusted);
            i += l;
            block_buffer.push_back(TypedBlock { uint8_t(l), uint8_t(type) });
        }

        void check_advanced(size_t len) {
            DCHECK_EQ(i - last_i, len);
            last_i = i;
        }

        std::vector<TypedBlock>& adjusted_blocks() {
            check_sizes("pre adjust");
            adjust_blocks(block_buffer);
            check_sizes("post adjust");

            print_all(true);
            for (auto& b: block_buffer) {
                DCHECK_GE(b.len, 2);
                DCHECK_LE(b.len, 3);
            }
            return block_buffer;
        }
    };
}}
