#pragma once

#include <unordered_set>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/util/rollinghash/rabinkarphash.hpp>

namespace tdc {

class EscapingSparseFactorCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(TypeDesc("sparse_factor_coder"), "escaping_sparse_factor_coder");
        m.param("escape_byte").primitive(255);
        return m;
    }

    class Coder {
        uint8_t m_escape_byte = 0;
        BitOStream m_stream;
    public:
        inline Coder(Config const& cfg, Output& output): m_stream(output) {
            m_escape_byte = cfg.param("escape_byte").as_uint();
        }

        inline void code_plain(View view) {
            for (uint8_t byte : view) {
                m_stream.write_int(byte);
                if (byte == m_escape_byte) {
                    m_stream.write_compressed_int(0);
                }
            }
        }

        inline void code_factor(size_t rel_position, size_t size) {
            DCHECK_NE(rel_position, 0);
            m_stream.write_int(m_escape_byte);
            m_stream.write_compressed_int(rel_position);
            m_stream.write_compressed_int(size);
        }
    };

    class Decoder {
        uint8_t m_escape_byte = 0;
        BitIStream m_stream;

    public:
        inline Decoder(Config const& cfg, Input& input): m_stream(input) {
            m_escape_byte = cfg.param("escape_byte").as_uint();
        }

        template<typename ByteF, typename FactorF>
        inline void decode_all(ByteF f, FactorF g) {
            while(!m_stream.eof()) {
                uint8_t byte = m_stream.read_int<uint8_t>();
                if (byte == m_escape_byte) {
                    size_t rel_position = m_stream.read_compressed_int<size_t>();
                    if (rel_position != 0) {
                        size_t size = m_stream.read_compressed_int<size_t>();
                        g(rel_position, size);
                        continue;
                    }
                }
                f(byte);
            }
        }
    };


};

template<typename sparse_factor_coder_t>
class LongCommonStringCompressor: public Compressor {
    using map_hash_t = size_t; // given by unordered_map API

    struct Offset {
        size_t m_end;
        map_hash_t m_hash;

        inline Offset(size_t offset, map_hash_t hash):
            m_end(offset),
            m_hash(hash) {}
    };

    struct HashFn {
        std::size_t operator()(const Offset& k) const {
            return k.m_hash;
        }
    };

    struct EqFn {
        View m_input;
        size_t m_b;

        inline bool operator()( const Offset& lhs, const Offset& rhs ) const {
            return m_input.slice(lhs.m_end - m_b, lhs.m_end) == m_input.slice(rhs.m_end - m_b, rhs.m_end);
        }
    };

    using map_t = std::unordered_multiset<Offset, HashFn, EqFn>;
    static constexpr size_t MAP_HASH_BITS = sizeof(map_hash_t) * CHAR_BIT;
    static constexpr size_t INITIAL_BUCKETS = 1;

    class Match {
        size_t m_src_begin;
        size_t m_dst_begin;
        size_t m_size;
    public:
        inline Match(size_t src_begin, size_t dst_begin, size_t size):
            m_src_begin(src_begin),
            m_dst_begin(dst_begin),
            m_size(size) {}

        inline size_t size() const { return m_size; }
        inline size_t src_begin() const { return m_src_begin; }
        inline size_t dst_begin() const { return m_dst_begin; }
        inline size_t src_end() const { return m_src_begin + m_size; }
        inline size_t dst_end() const { return m_dst_begin + m_size; }
    };

public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "long_common_string");
        m.param("sparse_factor_coder").strategy<sparse_factor_coder_t>(
            TypeDesc("sparse_factor_coder"),
            Meta::Default<EscapingSparseFactorCoder>());
        m.param("b").primitive(20);
        return m;
    }

    inline LongCommonStringCompressor(Config&& cfg)
        : Compressor(std::move(cfg)) {
    }

    inline virtual void compress(Input& input, Output& output) override final {
        size_t b = config().param("b").as_uint();
        CHECK_GT(b, 0);

        // TODO: Vary hash bit width?
        auto rolling_hash = rollinghash::KarpRabinHash<map_hash_t, uint8_t> {
            int(b),
            MAP_HASH_BITS
        };

        auto view = input.as_view();

        auto coder = typename sparse_factor_coder_t::Coder {
            config().sub_config("sparse_factor_coder"),
            output,
        };

        if (b >= view.size()) {
            // Don't bother to even start, we never hash a single block
            coder.code_plain(view);
        } else {
            // Hash the first b chars, to prepare the rolling hash state

            for(size_t i = 0; i < b; i++) {
                uint8_t byte = view[i];

                rolling_hash.eat(byte);
            }

            // Construct the hashset, and start the main loop

            auto map = map_t(INITIAL_BUCKETS, HashFn { }, EqFn { view, b });

            auto store = [&](size_t i) {
                map.insert(Offset { i, rolling_hash.hashvalue });
            };

            auto update_rolling_hash = [&](size_t i) {
                uint8_t old_byte = view[i - b];
                uint8_t new_byte = view[i];

                rolling_hash.update(old_byte, new_byte);
            };

            auto extend_match = [&](size_t const dst_left_border,
                                    size_t const src_end,
                                    size_t const dst_end) -> Match
            {
                size_t const dst_begin = dst_end - b;
                size_t const src_begin = src_end - b;

                // At this point, we know that there is a exact match of `b` bytes
                // to the left of `{src,dst}_end`, and
                // that `dst_left_border` might lie anywhere before or after `dst_end`.
                //
                // The goal of this function is to try to extend at most `b - 1` more bytes to the left
                // of the block, and as far as possible to the right.
                //
                // Because `dst_left_border` can lie anywhere, this means any of these scenarios are possible:
                //
                // # A block match with left and right extension
                //   [__left_border_    [ left ][  block  ][ right ]             ]
                //
                // # Left extension cut short by a previous right-extension
                //   [__________left_border_[  ][  block  ][ right ]             ]
                //
                // # Block cut short by a previous right-extension
                //   [___________________left_border_[    ][ right ]             ]
                //
                // # Entire match is already covered by a previous right-extension
                //   [_________________________________________left_border_      ]
                //

                // how many bytes to extend from {dst/src}_end
                size_t left_extend = 0;
                size_t right_extend = 0;

                // We only need to search for a left extension
                // if a previous right extension one would not have covered it already.
                if (dst_left_border < dst_begin) {
                    // Search backwards up-to b-1 elements
                    size_t const left_extend_begin
                        = std::max<size_t>(dst_left_border, dst_begin - (b - 1));
                    size_t const max_left_extend = dst_begin - left_extend_begin;
                    left_extend = max_left_extend + b;
                    for(size_t i = 0; i < max_left_extend; i++) {
                        if ((src_begin - i == 0) || (view[dst_begin - i - 1] != view[src_begin - i - 1])) {
                            left_extend = i + b;
                            break;
                        }
                    }
                } else {
                    if (dst_left_border < dst_end) {
                        left_extend = dst_end - dst_left_border;
                    } else {
                        left_extend = 0;
                    }
                }

                // We only need to search for a right extension
                // if a previous right extension one would not have covered it already.
                if (dst_left_border < dst_end) {
                    // Search forward
                    size_t const max_right_extend = view.size() - dst_end;
                    right_extend = max_right_extend;
                    for(size_t i = 0; i < max_right_extend; i++) {
                        if (view[dst_end + i] != view[src_end + i]) {
                            right_extend = i;
                            break;
                        }
                    }
                }

                size_t const src_match_begin = src_end - left_extend;
                size_t const dst_match_begin = dst_end - left_extend;
                size_t const match_size = left_extend + right_extend;

                return { src_match_begin, dst_match_begin, match_size };
            };

            auto checkformatch = [&](size_t left_border, size_t i) -> Match {
                auto dst = Offset { i, rolling_hash.hashvalue };
                auto closest_max_match = Match { 0, 0, 0 };

                // Iterate over all previously seen occurrences of this fingerprint
                for(auto pair = map.equal_range(dst); pair.first != pair.second; ++pair.first) {
                    //create and extend match as far as possible to the left and right
                    auto match = extend_match(left_border, pair.first->m_end, dst.m_end);

                    if (match.size() > closest_max_match.size()) {
                        closest_max_match = match;
                    } else if (match.size() == closest_max_match.size()) {
                        // NB: This case is optional, and not described in the original
                        // algorithm.
                        // It ensures that we find the largest match _closest_
                        // to the current location, to minimize the bitsize of
                        // the back-reference.

                        if (match.src_begin() > closest_max_match.src_begin()) {
                            closest_max_match = match;
                        }
                    }
                }

                return closest_max_match;
            };

            size_t last_output_offset = 0;
            auto flush_plain = [&](size_t to, string_ref) {
                if (last_output_offset < to) {
                    coder.code_plain(view.slice(last_output_offset, to));
                    last_output_offset = to;
                }
            };

            for(size_t i = b; i < view.size();) {
                if (i % b == 0) {
                    // store a hash for the current block
                    store(i);
                }
                update_rolling_hash(i);

                i++;

                // check if we can find the current b chars in the hashmap,
                // and if yes, how many (extended) bytes did we match
                auto res = checkformatch(last_output_offset, i);
                if (res.size() != 0) {
                    flush_plain(res.dst_begin(), "intra-unmatched");

                    // We encode the relative reference to the current position
                    coder.code_factor(res.dst_begin() - res.src_begin(), res.size());

                    // Continue further search from the end of the match
                    last_output_offset = res.dst_end();
                }
            }
            flush_plain(view.size(), "post-unmatched");
        }
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        std::vector<uint8_t> buffer;

        auto decoder = typename sparse_factor_coder_t::Decoder {
            config().sub_config("sparse_factor_coder"),
            input,
        };

        size_t i = 0;
        decoder.decode_all([&](uint8_t byte) {
            buffer.push_back(byte);
            i++;
        }, [&](size_t rel_position, size_t size) {
            size_t position = i - rel_position;
            size_t end_position = position + size;
            while (position != end_position) {
                buffer.push_back(buffer[position++]);
            }
            i += size;
        });

        auto o = output.as_stream();
        o << View(buffer);
    }
};

}
