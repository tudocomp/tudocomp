#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/landmarks.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/esp_math.hpp>
#include <tudocomp/compressors/esp/RoundContext.hpp>

namespace tdc {
namespace esp {

template<typename Source>
inline void eager_mb13(const Source& src, RoundContext<Source>& ctx, size_t t) {
    [&]() {
        size_t j = src.size();
        for (size_t i = 0; i < j;) {
            size_t remaining_len = j - i;
            switch (remaining_len) {
                case 4:
                    ctx.push_back(2, t);
                    ctx.push_back(2, t);
                    return;
                case 3:
                    ctx.push_back(3, t);
                    return;
                case 2:
                    ctx.push_back(2, t);
                    return;
                case 1:
                    ctx.push_back(1, t);
                    //DCHECK_GT(remaining_len, 1);
                    return;
                case 0:
                    return;
                default:
                    ctx.push_back(3, t);
                    i += 3;
            }
        }
    }();
    ctx.check_advanced(src.size());
}

template<class T, class F>
void for_neigbors2(T& t, F f) {
    for (size_t i = 0; i < t.size(); i++) {
        std::array<typename T::value_type, 2> neighbors;
        uint8_t neighbor_len = 0;

        if (i == 0 && i == t.size() - 1) {
            neighbor_len = 0;
        } else if (i == 0) {
            neighbor_len = 1;
            neighbors[0] = t[i + 1];
        } else if (i == t.size() - 1) {
            neighbor_len = 1;
            neighbors[0] = t[i - 1];
        } else {
            neighbor_len = 2;
            neighbors[0] = t[i - 1];
            neighbors[1] = t[i + 1];
        }

        f(i, ConstGenericView<typename T::value_type>(neighbors.data(), neighbor_len));
    }
}

template<typename Source>
inline void eager_mb2(const Source& src, RoundContext<Source>& ctx) {
    auto A = src;
    DCHECK(A.size() > 0);
    auto type_3_prefix_len = std::min(iter_log(ctx.alphabet_size),
                                      A.size());

    // Handle non-m2 prefix
    {
        auto type_3_prefix = A.substr(0, type_3_prefix_len);
        eager_mb13(type_3_prefix, ctx, 3);
        if (type_3_prefix_len == A.size()) { return; }
    }

    auto type_2_suffix_size = src.size() - type_3_prefix_len;

    // Prepare scratchpad buffer
    auto& buf = ctx.scratchpad;
    buf.clear();
    buf.reserve(A.cend() - A.cbegin());
    buf.insert(buf.cbegin(), A.cbegin(), A.cend());

    // Iterate on the buffer by combing each two adjacent elements.
    // This reduces the size by `iter_log(alphabet_size) == type_3_prefix_len`
    // the alphabet to size 6.
    {
        IF_DEBUG(if (ctx.print_mb2_trace) {
            std::cout << "  " << "Initial:" << "\n";
            std::cout << "  " << vec_to_debug_string(buf) << "\n";
            std::cout << "  " << "Reduce to 6:\n";
        })
        for (uint shrink_i = 0; shrink_i < type_3_prefix_len; shrink_i++) {
            for (size_t i = 1; i < buf.size(); i++) {
                auto left  = buf[i - 1];
                auto right = buf[i];
                buf[i - 1] = label(left, right);
            }
            buf.pop_back();

            IF_DEBUG(if (ctx.print_mb2_trace) {
                std::cout << "  " << vec_to_debug_string(buf) << "\n";
            })
        }
        IF_DEBUG(
            DCHECK_LE(calc_alphabet_size(buf), 6);
        )
    }

    // Reduce further to alphabet 3
    {
        IF_DEBUG(if (ctx.print_mb2_trace) {
            std::cout << "  " << "Reduce to 3:\n";
        })

        // TODO: This would benefit from a general, mutable, slice type

        // final pass: reduce to alphabet 3
        for(uint to_replace = 3; to_replace < 6; to_replace++) {
            for_neigbors2(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
                auto& e = buf[i];
                if (e == to_replace) {
                    e = 0;
                    for (auto n : neighbors) { if (n == e) { e++; } }
                    for (auto n : neighbors) { if (n == e) { e++; } }
                }
            });

            IF_DEBUG(if (ctx.print_mb2_trace) {
                std::cout << "  " << vec_to_debug_string(buf) << "\n";
            })
        }

        IF_DEBUG(
            DCHECK(calc_alphabet_size(buf) <= 3);
            DCHECK(no_adjacent_identical(buf));
        )
    }

    // find landmarks:
    {
        // TODO: Maybe store in high bits of buf to reduce memory?
        // buf gets reduced to 2 bit values anyway, and stays around long enough
        IntVector<uint_t<1>> landmarks(buf.size());

        for_neigbors2(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
            bool is_high_landmark = true;
            for (auto e : neighbors) {
                if (e > buf[i]) {
                    is_high_landmark = false;
                }
            }
            if (is_high_landmark) {
                landmarks[i] = 1;
            }
        });

        IF_DEBUG(if (ctx.print_mb2_trace) {
            std::cout << "  High Landmarks:\n";
            std::cout << "  " << vec_to_debug_string(landmarks) << "\n";
        })

        for_neigbors2(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
            bool is_low_landmark = true;
            for (auto e : neighbors) {
                if (e < buf[i]) {
                    is_low_landmark = false;
                }
            }
            // if there is a large enough landmark-less gap, mark it as well
            if (is_low_landmark) {
                //if (i > 0 && i < buf.size() - 1)
                if (   (!(i > 0)              || (landmarks[i - 1] == 0u))
                    && (!(i < buf.size() - 1) || (landmarks[i + 1] == 0u))
                ) {
                    landmarks[i] = 1;
                }
            }
        });

        IF_DEBUG(if (ctx.print_mb2_trace) {
            std::cout << "  High and Low Landmarks:\n";
            std::cout << "  " << vec_to_debug_string(landmarks) << "\n";
        })

        IF_DEBUG(
            DCHECK(check_landmarks(landmarks, true));
        )

        // Split at landmarks

        // TODO: Make not need puffer
        IF_DEBUG(
            std::vector<size_t> debug_landmark_assoc(buf.size());
            size_t debug_landmark_counter = 2;
        )

        landmark_spanner(
            landmarks.size(),
            [&](size_t i) {
                return landmarks[i] == uint_t<1>(1);
            }, [&](size_t left, size_t right) {
                ctx.push_back(right - left + 1, 2);
                IF_DEBUG(
                    for (size_t j = left; j <= right; j++) {
                        debug_landmark_assoc[j] = debug_landmark_counter;
                    }
                    debug_landmark_counter++;
                )
            },
            ctx.behavior_landmarks_tie_to_right
        );

        IF_DEBUG(if (ctx.print_mb2_trace) {
            std::cout << "  Block-Landmark Assignment New:\n";
            std::cout << "  " << vec_to_debug_string(debug_landmark_assoc) << "\n";
        })
    }

    ctx.check_advanced(type_2_suffix_size);
}

template<typename Source, typename F>
inline size_t split_where(const Source& src, size_t i, bool max, F f) {
    for(size_t j = i; j < src.size() - 1; j++) {
        if (!f(src[j], src[j + 1])) {
            return j + (max ? 1 : 0);
        }
    }
    return src.size();
}

template<typename Source>
inline void split(const Source& src, RoundContext<Source>& ctx) {
    // Split up the input into metablocks of type 2 or 1/3
    for (size_t i = 0; i < src.size();) {
        size_t j;

        // Scan for non-repeating
        // NB: First to not find a size-1 repeating prefix
        j = split_where(src, i, !ctx.behavior_metablocks_maximimze_repeating,
                        [](size_t a, size_t b){ return a != b; });
        if(j != i) {
            auto s = src.slice(i, j);
            IF_DEBUG(if (!ctx.print_only_adjusted && ctx.print_mb_trace) {
                std::cout << std::setw(13 + i) << ""
                    << debug_p(s, ctx.alphabet_size)
                    << "\n";
            })
            eager_mb2(s, ctx);
            i = j;
        }

        // Scan for repeating
        j = split_where(src, i, ctx.behavior_metablocks_maximimze_repeating,
                        [](size_t a, size_t b){ return a == b; });
        if(j != i) {
            auto s = src.slice(i, j);
            IF_DEBUG(if (!ctx.print_only_adjusted && ctx.print_mb_trace) {
                std::cout << std::setw(13 + i) << ""
                    << debug_p(s, ctx.alphabet_size)
                    << "\n";
            })
            eager_mb13(s, ctx, 1);
            i = j;
        }
    }
}

}
}
