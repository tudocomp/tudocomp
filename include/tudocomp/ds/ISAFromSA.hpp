#pragma once

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/ArrayDS.hpp>
#include "SparseISA.hpp"

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
class ISAFromSA {
private:
	SparseISA SA;
public:
    inline static Meta meta() {
        Meta m("isa", "from_sa");
        return m;
    }

    template<typename textds_t>
    inline ISAFromSA(Env&& env, textds_t& t, CompressMode cm) : SA(env, t, cm) {
    }

    inline int operator[](size_t i) const {
        return SA.getInv(i);
    }

    void compress() {
    /*    DCHECK(m_data);

        env().begin_stat_phase("Compress ISA");

        m_data->width(bits_for(m_data->size()));
        m_data->shrink_to_fit();

        env().log_stat("bit_width", size_t(m_data->width()));
        env().log_stat("size", m_data->bit_size() / 8);
        env().end_stat_phase();*/
    }
};

} //ns
