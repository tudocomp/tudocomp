#pragma once

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

/// \brief Base for data structures that use an integer array as a storage.
class ArrayDS: public DynamicIntVector {
protected:
    IF_DEBUG(
        /// Debug check to ensure the vector has not been moved out.
        bool m_is_initialized = false;
    )

    /// \brief The type of integer array to use as storage.
    using iv_t = DynamicIntVector;

    inline void debug_check_array_is_initialized() const {
        IF_DEBUG(
            DCHECK(m_is_initialized);
        )
    }

    inline void set_array(iv_t&& iv) {
        (iv_t&)(*this) = std::move(iv);
        IF_DEBUG(m_is_initialized = true;)
    }
public:
    inline ArrayDS() {}
    inline ArrayDS(const ArrayDS& other) = delete;
    inline ArrayDS(ArrayDS&& other): DynamicIntVector(std::move(other)){
        IF_DEBUG(m_is_initialized = other.m_is_initialized;)
    }
    inline ArrayDS& operator=(ArrayDS&& other) {
        set_array(std::move(other));
        IF_DEBUG(m_is_initialized = other.m_is_initialized;)
        return *this;
    }

    /// \brief The data structure's data type.
    using data_type = iv_t;

    /// \brief Forces the data structure to relinquish its data storage.
    ///
    /// This is done by moving the ownership of the storage to the caller.
    /// After this operation, the data structure will behave as if it was
    /// empty, and may throw debug assertions on access.
    inline iv_t relinquish() {
        debug_check_array_is_initialized();
        IF_DEBUG(m_is_initialized = false;)
        return std::move(static_cast<iv_t&>(*this));
    }

    /// \brief Creates a copy of the data structure's storage.
    inline iv_t copy() const {
        debug_check_array_is_initialized();
        return iv_t(*this);
    }

};

} //ns
