#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/ds/rank/rank_64bit.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

/// \brief Implements a rank data structure for a \ref BitVector.
///
/// The data structure follows the block / superblock principle. Blocks are
/// always 64 bits in size; the size of a superblock defaults to 4096, but
/// is optional.
class Rank {
public:
    static constexpr size_t block_size =
        8 * sizeof(BitVector::internal_data_type);

    static_assert(block_size <= 64, "block_size cannot be larger than 64 bits");

private:
    const BitVector* m_bv;

    size_t m_supblock_size;
    DynamicIntVector m_blocks;
    DynamicIntVector m_supblocks;

public:
    inline Rank()
        : m_bv(nullptr),
          m_supblock_size(0) {
    }

    inline Rank(const Rank& other)
        : m_bv(other.m_bv),
          m_supblock_size(other.m_supblock_size),
          m_blocks(other.m_blocks),
          m_supblocks(other.m_supblocks) {
    }

    inline Rank(Rank&& other)
        : m_bv(other.m_bv),
          m_supblock_size(other.m_supblock_size),
          m_blocks(std::move(other.m_blocks)),
          m_supblocks(std::move(other.m_supblocks)) {
    }

    inline Rank& operator=(const Rank& other) {
        m_bv = other.m_bv;
        m_supblock_size = other.m_supblock_size;
        m_blocks = other.m_blocks;
        m_supblocks = other.m_supblocks;
        return *this;
    }

    inline Rank& operator=(Rank&& other) {
        m_bv = other.m_bv;
        m_supblock_size = other.m_supblock_size;
        m_blocks = std::move(other.m_blocks);
        m_supblocks = std::move(other.m_supblocks);
        return *this;
    }

    inline Rank(
        const BitVector& bv,
        size_t supblock_size = block_size * block_size)
        : m_bv(&bv),
          m_supblock_size(supblock_size) {

        DCHECK_GT(supblock_size, block_size)
            << "superblocks must be larger than blocks!";
        DCHECK_EQ(0, supblock_size % block_size)
            << "superblock size must be a multiple of the block size!";

        const size_t n = m_bv->size();
        const auto data = m_bv->data();

        // compute number of superblocks / blocks
        // note that we do not need to round up, as the first block always
        // contains zero and does not need to be stored
        const size_t num_supblocks = n / supblock_size;
        const size_t num_blocks    = n / block_size;

        const size_t blocks_per_supblock = supblock_size / block_size;

        m_supblocks = DynamicIntVector(num_supblocks, 0, bits_for(n));
        m_blocks = DynamicIntVector(num_blocks, 0, bits_for(supblock_size));

        // construct
        size_t rank_bv = 0; // 1-bits in whole BV
        size_t rank_sb = 0; // 1-bits in current superblock
        size_t cur_sb = 0;   // current superblock

        for(size_t j = 0; j < num_blocks; j++) {
            size_t i = j / blocks_per_supblock;
            if(i > cur_sb) {
                // we reached a new superblock
                m_supblocks[cur_sb] = rank_bv;
                rank_sb = 0;
                cur_sb = i;
            }

            auto rank_b = tdc::rank1(data[j]);
            rank_sb += rank_b;
            rank_bv += rank_b;

            m_blocks[j] = rank_sb;
        }
    }

    /// rank1 for [0, x]
    inline size_t rank1(size_t x) const {
        DCHECK_LT(x, m_bv->size());
        size_t r = 0;
        size_t i = x / m_supblock_size;
        if(i > 0) r += m_supblocks[i-1];
        size_t j = x / block_size;

        size_t k = j - i * (m_supblock_size / block_size);
        if(k > 0) r += m_blocks[j-1];

        r += tdc::rank1(m_bv->data()[j], x % block_size);
        return r;
    }

    /// rank1 for [x, y]
    inline size_t rank1(size_t x, size_t y) const {
        DCHECK_LE(x, y);
        size_t r = rank1(y);
        if(x > 0) r -= rank1(x-1);
        return r;
    }

    inline size_t operator()(size_t x) const {
        return rank1(x);
    }

    inline size_t operator()(size_t x, size_t y) const {
        return rank1(x, y);
    }

    inline size_t rank0(size_t x) const {
        return x + 1 - rank1(x);
    }

    inline size_t rank0(size_t x, size_t y) const {
        return (y - x + 1) - rank1(x, y);
    }
};

}
