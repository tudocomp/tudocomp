#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/ds/rank/rank_64bit.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

class Rank {
private:
    DynamicIntVector m_blocks;
    DynamicIntVector m_super_blocks;

public:
    inline Rank(const BitVector& bv) {
        const size_t n = bv.size();
        const size_t b = 64;
        const size_t sb = b * b;

        LOG(INFO) << "b = " << b << ", sb = " << sb;

        m_super_blocks = DynamicIntVector(n / sb, 0, bits_for(n));
        m_blocks = DynamicIntVector(n / b,  0, bits_for(sb));

        LOG(INFO) << "|BV| = " << bv.size();
        LOG(INFO) << "|M'| = " << m_super_blocks.size() << ", |M| = " << m_blocks.size();
        LOG(INFO) << "total size in bits: " << (m_super_blocks.bit_size() + m_blocks.bit_size());

        LOG(INFO) << "constructing ...";
        const uint64_t* data = bv.data();
        size_t rank1_bv = 0;
        for(size_t i = 0; i < (n/sb); i++) {
            size_t rank1_sb = 0;
            for(size_t j = 0; j < sb && (i * sb + j) < (n/b); j++) {
                rank1_sb += rank1(data[i * sb + j]);
                m_blocks[i * sb + j] = rank1_sb;
            }
            rank1_bv += rank1_sb;
            m_super_blocks[i] = rank1_bv;
        }
        LOG(INFO) << "Done.";
    }
};

}

