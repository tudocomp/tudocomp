#pragma once

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

/// \brief Base for data structures that use an integer array as a storage.
class ArrayDS : public Algorithm {
protected:
    /// \brief The type of integer array to use as storage.
    using iv_t = DynamicIntVector;

    /// \brief The integer array used as storage.
    std::unique_ptr<iv_t> m_data;

public:
    /// \brief The data structure's data type.
    using data_type = iv_t;

    using Algorithm::Algorithm;

    /// \brief Forces the data structure to relinquish its data storage.
    ///
    /// This is done by moving the ownership of the storage to the caller.
    /// After this operation, the data structure will behave as if it was
    /// uninitialized.
    inline std::unique_ptr<iv_t> relinquish() {
        DCHECK(m_data);
        return std::move(m_data);
    }

    /// \brief Creates a copy of the data structure's storage.
    inline std::unique_ptr<iv_t> copy() {
        DCHECK(m_data);
        return std::make_unique<iv_t>(*m_data);
    }

    /// \brief Accesses an element in the data structure.
    ///
    /// \param i The index of the element to access.
    inline len_t operator[](size_t i) const {
        DCHECK(m_data);
        return (*m_data)[i];
    }

    /// \brief Yields the number of elements stored in this data structure.
    /// \return The number of elements stored in this data structure.
    inline size_t size() const {
        DCHECK(m_data);
        return m_data->size();
    }
};

} //ns
