#pragma once

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

class ArrayDS : public Algorithm {
protected:
    using iv_t = DynamicIntVector;
    std::unique_ptr<iv_t> m_data;

public:
    using data_type = iv_t;
    using Algorithm::Algorithm;

    inline std::unique_ptr<iv_t> relinquish() {
        DCHECK(m_data);
        return std::move(m_data);
    }

    inline std::unique_ptr<iv_t> copy() {
        DCHECK(m_data);
        return std::make_unique<iv_t>(*m_data);
    }

    inline len_t operator[](size_t i) const {
        DCHECK(m_data);
        return (*m_data)[i];
    }

    inline size_t size() const {
        DCHECK(m_data);
        return m_data->size();
    }
};

} //ns
