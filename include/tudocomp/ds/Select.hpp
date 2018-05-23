#pragma once

#include <tudocomp/ds/select_64bit.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/Rank.hpp>

namespace tdc {

/// \brief Implements a select data structure for a \ref BitVector.
///
/// The data structure follows a block / superblock principle, where superblocks
/// always contain \c s flagged bits and blocks contain \c s' flagged bits
/// (flagged bits meaning 1 for select1 and 0 for select0).
///
/// The size of a block is logarithmic in the bit vector's length, the size of a
/// superblock is the squared block size.
///
/// \tparam m_bit the bits to flag (0 or 1)
template<bool m_bit>
class Select {
private:
    using data_t = BitVector::internal_data_type;
    static constexpr size_t data_w = 8 * sizeof(data_t);

    static_assert(data_w <= 64,
        "bit vectors backing must have width of at most 64 bits");

    // template variants
    static constexpr uint8_t basic_rank(data_t v);
    static constexpr uint8_t basic_rank(data_t v, uint8_t l, uint8_t m);
    static constexpr uint8_t basic_select(data_t, uint8_t k);
    static constexpr uint8_t basic_select(data_t, uint8_t l, uint8_t k);

    const BitVector* m_bv;

    size_t m_max;
    size_t m_block_size;
    size_t m_supblock_size;

    DynamicIntVector m_blocks;
    DynamicIntVector m_supblocks;

public:
    /// \brief Default constructor.
    inline Select()
        : m_bv(nullptr),
          m_max(0),
          m_block_size(0),
          m_supblock_size(0) {
    }

    /// \brief Copy constructor.
    inline Select(const Select& other)
        : m_bv(other.m_bv),
          m_max(other.m_max),
          m_block_size(other.m_block_size),
          m_supblock_size(other.m_supblock_size),
          m_blocks(other.m_blocks),
          m_supblocks(other.m_supblocks) {
    }

    /// \brief Move constructor.
    inline Select(Select&& other)
        : m_bv(other.m_bv),
          m_max(other.m_max),
          m_block_size(other.m_block_size),
          m_supblock_size(other.m_supblock_size),
          m_blocks(std::move(other.m_blocks)),
          m_supblocks(std::move(other.m_supblocks)) {
    }

    /// \brief Copy assignment.
    inline Select& operator=(const Select& other) {
        m_bv = other.m_bv;
        m_max = other.m_max;
        m_block_size = other.m_block_size;
        m_supblock_size = other.m_supblock_size;
        m_blocks = other.m_blocks;
        m_supblocks = other.m_supblocks;
        return *this;
    }

    /// \brief Move assignment.
    inline Select& operator=(Select&& other) {
        m_bv = other.m_bv;
        m_max = other.m_max;
        m_block_size = other.m_block_size;
        m_supblock_size = other.m_supblock_size;
        m_blocks = std::move(other.m_blocks);
        m_supblocks = std::move(other.m_supblocks);
        return *this;
    }

    /// \brief Constructs the select data structure for the given bit vector.
    ///
    /// Note that changes to the bit vector after construction of this data
    /// structure will cause the select operation to not work correctly
    /// anymore. In other words, this data structure is static.
    ///
    /// \param bv the underlying bit vector
    inline Select(BitVector& bv) : m_bv(&bv) {
        const size_t n = bv.size();

        const size_t log_n = bits_for(n);

        // construct
        m_supblock_size = log_n * log_n;
        m_block_size    = log_n;

        m_supblocks = DynamicIntVector(idiv_ceil(n, m_supblock_size), 0, log_n);
        m_blocks = DynamicIntVector(idiv_ceil(n, m_block_size), 0, log_n);

        m_max = 0;
        size_t r_sb = 0; // current bit count in superblock
        size_t r_b = 0;  // current bit count in block

        size_t cur_sb = 0;        // current superblock
        size_t cur_sb_offset = 0; // starting position of current superblock
        size_t longest_sb = 0;    // length of longest superblock

        size_t cur_b = 0; // current block

        auto data = bv.data();
        const size_t num_data = idiv_ceil(n, data_w);

        for(size_t i = 0; i < num_data; i++) {
            const auto v = data[i];

            uint8_t r;
            if(i + 1 < num_data) {
                // get amount flag bits in whole data element
                r = basic_rank(v);
            } else {
                // only get amount of flag bits up to end of bit vector
                const size_t m = n % data_w;
                r = basic_rank(v, 0, m-1);
            }

            m_max += r;

            if(r_b + r >= m_block_size) {
                // entered new block

                // amount of bits needed to fill current block
                size_t distance_b = m_block_size - r_b;

                // stores the offset of the last bit in the current block
                uint8_t offs = 0;

                r_b += r;

                size_t distance_sum = 0;
                while(r_b >= m_block_size) {
                    // find exact position of the bit in question
                    offs = basic_select(v, offs, distance_b);
                    DCHECK_NE(SELECT_FAIL, offs);

                    const size_t pos = i * data_w + offs;

                    distance_sum += distance_b;
                    r_sb += distance_b;
                    if(r_sb >= m_supblock_size) {
                        // entered new superblock
                        longest_sb = std::max(longest_sb, pos - cur_sb_offset);
                        cur_sb_offset = pos;

                        m_supblocks[cur_sb++] = pos;

                        r_sb -= m_supblock_size;
                    }

                    m_blocks[cur_b++] = pos - cur_sb_offset;
                    r_b -= m_block_size;
                    distance_b = m_block_size;

                    ++offs;
                }

                DCHECK_GE(size_t(r), distance_sum);
                r_sb += r - distance_sum;
            } else {
                r_b  += r;
                r_sb += r;
            }
        }

        longest_sb = std::max(longest_sb, n - cur_sb_offset);
        const size_t w_block = bits_for(longest_sb);

        m_blocks.resize(cur_b);
        m_blocks.width(w_block);
        m_blocks.shrink_to_fit();

        m_supblocks.resize(cur_sb);
        m_supblocks.shrink_to_fit();
    }

    /// \brief Finds the position of the x-th flagged bit in the bit vector.
    /// \param x the order of the flagged bit to find
    /// \return the position of the x-th flagged bit. In case the position is
    ///         not contained in the bit vector, the bit vector's size is
    ///         returned.
    inline size_t select(size_t x) const {
        DCHECK_GT(x, 0) << "order must be at least one";
        if(x > m_max) return m_bv->size();

        size_t pos = 0;

        //narrow down to block
        {
            const size_t i = x / m_supblock_size;
            const size_t j = x / m_block_size;

            if(i > 0) {
                pos += m_supblocks[i-1];
                x -= i * m_supblock_size;
            }
            if(x == 0) return pos;

            // block j is the k-th block within the i-th superblock
            size_t k = j - i * (m_supblock_size / m_block_size);
            if(k > 0) {
                pos += m_blocks[j-1];
                x   -= k * m_block_size;
            }
            if(x == 0) return pos;

            if(i > 0 || k > 0) ++pos; // offset from block boundary
        }

        // from this point forward, search directly in the bit vector
        auto data = m_bv->data();

        size_t i = pos / data_w;
        size_t offs  = pos % data_w;

        uint8_t s = basic_select(data[i], offs, x);
        if(s != SELECT_FAIL) {
            // found in first data segment
            return pos + s - offs;
        } else {
            // linearly search in the next segments
            size_t passes = 1;

            x -= basic_rank(data[i], offs, data_w-1);
            do {
                ++passes;
                pos = (++i) * data_w;
                s = basic_select(data[i], x);
                if(s == SELECT_FAIL) x -= basic_rank(data[i]);
            } while(s == SELECT_FAIL);

            return pos + s;
        }
    }

    /// \see select
    inline size_t operator()(size_t k) const {
        return select(k);
    }
};

using Select1 = Select<1>;

/// \cond INTERNAL
template<>
inline constexpr uint8_t Select1::basic_rank(data_t v) {
    return tdc::rank1(v);
}

template<>
inline constexpr uint8_t Select1::basic_rank(data_t v, uint8_t l, uint8_t m) {
    return tdc::rank1(v, l, m);
}

template<>
inline constexpr uint8_t Select1::basic_select(data_t v, uint8_t k) {
    return tdc::select1(v, k);
}

template<>
inline constexpr uint8_t Select1::basic_select(data_t v, uint8_t l, uint8_t k) {
    return tdc::select1(v, l, k);
}
///\endcond

using Select0 = Select<0>;

/// \cond INTERNAL
template<>
inline constexpr uint8_t Select0::basic_rank(data_t v) {
    return tdc::rank0(v);
}

template<>
inline constexpr uint8_t Select0::basic_rank(data_t v, uint8_t l, uint8_t m) {
    return tdc::rank0(v, l, m);
}

template<>
inline constexpr uint8_t Select0::basic_select(data_t v, uint8_t k) {
    return tdc::select0(v, k);
}

template<>
inline constexpr uint8_t Select0::basic_select(data_t v, uint8_t l, uint8_t k) {
    return tdc::select0(v, l, k);
}
///\endcond

}
