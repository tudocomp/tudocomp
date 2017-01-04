#pragma once

#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

class ArrayDS : public Algorithm {
public:
    using iv_t = DynamicIntVector;

protected:
    std::unique_ptr<iv_t> m_data;

public:
    using Algorithm::Algorithm;

    inline iv_t& data() {
        DCHECK(m_data);
        return *m_data;
    }

    inline const iv_t& data() const {
        DCHECK(m_data);
        return *m_data;
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
