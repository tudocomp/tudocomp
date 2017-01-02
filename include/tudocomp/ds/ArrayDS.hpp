#pragma once

#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

class ArrayDS : public Algorithm {
public:
    using iv_t = DynamicIntVector;

protected:
    iv_t m_data;

public:
    using Algorithm::Algorithm;

    inline iv_t& data() {
        return m_data;
    }

    inline const iv_t& data() const {
        return m_data;
    }

    inline len_t operator[](size_t i) const {
        return m_data[i];
    }

    inline size_t size() const {
        return m_data.size();
    }
};

} //ns
