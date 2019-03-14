#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/landmarks.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/esp_math.hpp>
#include <tudocomp/compressors/esp/BlockGrid.hpp>

namespace tdc {namespace esp {

template<typename level_view_t>
class MetablockContext {
    BlockGrid* m_grid;
    std::vector<size_t>* m_scratchpad;
    size_t m_alphabet_size;
public:
    MetablockContext(std::vector<size_t>& scratchpad, BlockGrid& grid, size_t alphabet_size):
        m_grid(&grid), m_scratchpad(&scratchpad), m_alphabet_size(alphabet_size) {}

    inline void eager_mb13(const level_view_t& src, size_t t) {
        size_t j = src.size();
        for (size_t i = 0; i < j;) {
            size_t remaining_len = j - i;
            switch (remaining_len) {
                case 4:
                    m_grid->push_block(2, t);
                    m_grid->push_block(2, t);
                    return;
                case 3:
                    m_grid->push_block(3, t);
                    return;
                case 2:
                    m_grid->push_block(2, t);
                    return;
                case 1:
                    m_grid->push_block(1, t);
                    //DCHECK_GT(remaining_len, 1);
                    return;
                case 0:
                    return;
                default:
                    m_grid->push_block(3, t);
                    i += 3;
            }
        }
    }

    inline void eager_mb2(const level_view_t& src) {
        auto A = src;
        DCHECK_GT(A.size(), 0U);
        auto type_3_prefix_len = std::min(iter_log(m_alphabet_size),
                                        A.size());

        // Handle non-m2 prefix
        {
            auto type_3_prefix = A.slice(0, type_3_prefix_len);
            eager_mb13(type_3_prefix, 3);
            if (type_3_prefix_len == A.size()) { return; }
        }

        // Prepare scratchpad buffer
        auto& buf = *m_scratchpad;
        buf.clear();
        buf.reserve(A.cend() - A.cbegin());
        buf.insert(buf.cbegin(), A.cbegin(), A.cend());

        // Iterate on the buffer by combing each two adjacent elements.
        // This reduces the size by `iter_log(alphabet_size) == type_3_prefix_len`
        // the alphabet to size 6.
        {
            for (uint shrink_i = 0; shrink_i < type_3_prefix_len; shrink_i++) {
                for (size_t i = 1; i < buf.size(); i++) {
                    auto left  = buf[i - 1];
                    auto right = buf[i];
                    buf[i - 1] = label(left, right);
                }
                buf.pop_back();
            }

            DCHECK_LE(calc_alphabet_size(buf), 6U);
        }

        // Reduce further to alphabet 3
        {
            // final pass: reduce to alphabet 3
            for(uint to_replace = 3; to_replace < 6; to_replace++) {
                do_for_neighbors(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
                    auto& e = buf[i];
                    if (e == to_replace) {
                        e = 0;
                        for (auto n : neighbors) { if (n == e) { e++; } }
                        for (auto n : neighbors) { if (n == e) { e++; } }
                    }
                });
            }

            DCHECK_LE(calc_alphabet_size(buf), 3U);
            DCHECK(no_adjacent_identical(buf));
        }

        // find landmarks:
        {
            // TODO: Maybe store in high bits of buf to reduce memory?
            // buf gets reduced to 2 bit values anyway, and stays around long enough
            IntVector<uint_t<1>> landmarks(buf.size());

            do_for_neighbors(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
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

            do_for_neighbors(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
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

            DCHECK(check_landmarks(landmarks, true));

            // Split at landmarks

            landmark_spanner(
                landmarks.size(),
                [&](size_t i) {
                    return landmarks[i] == uint_t<1>(1);
                },
                [&](size_t left, size_t right) {
                    m_grid->push_block(right - left + 1, 2);
                }
            );
        }
    }
};

}}
