#pragma once

#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

/// \brief Represents a binary max heap backed by an external array of keys.
///
/// The max heap induces an order on the indices of the key array it is
/// backed by. The first element of the heap is the index of the largest item
/// in the key array.
///
/// \tparam array_t The key array type. Must support the `[]` operator.
template<typename array_t>
class ArrayMaxHeap {

private:
    enum perlocation_dir_t {
        NONE,
        LEFT,
        RIGHT
    };

    static inline len_t lc(len_t i) {
        return 2*i+1;
    }

    static inline len_t rc(len_t i) {
        return 2*i+2;
    }

    static inline len_t parent(len_t i) {
        return (i-1)/2;
    }

    // the array
    array_t* m_array;

    // undefined position in heap
    size_t m_undef;

    // heap
    size_t m_size;
    DynamicIntVector m_heap;

    // back mapping
    DynamicIntVector m_pos;

    inline void put(size_t pos, len_t i) {
        m_heap[pos] = i;
        m_pos[i] = pos;
    }

public:
    /// \brief Default constructor.
    ///
    /// Note that the constructor will not insert any items into the heap and
    /// serves merely for initialization.
    ///
    /// \param array The array of keys sorted by this heap.
    /// \param array_size The size of the key array.
    /// \param heap_size The maximum amount of items stored in the heap.
    inline ArrayMaxHeap(array_t& array, const size_t array_size, const size_t heap_size)
        : m_array(&array), m_size(0)
	{
        m_heap = DynamicIntVector(heap_size, 0, bits_for(array_size-1));
        m_undef = heap_size;
        m_pos = DynamicIntVector(array_size, m_undef, bits_for(m_undef));
    }

    /// \brief Inserts an item into the heap.
    ///
    /// \param i The index of the item in the key array. The key is retrieved
    ///          from there.
    inline void insert(len_t i) {
        DCHECK_EQ(m_pos[i], m_undef) << "trying to insert an item that's already in the heap";

        size_t pos = m_size++;

        // perlocate up
        auto lcp_i = (*m_array)[i];
        while(pos > 0 && lcp_i > (*m_array)[m_heap[parent(pos)]]) {
            put(pos, m_heap[parent(pos)]);
            pos = parent(pos);
        }

        put(pos, i);

        IF_PARANOID({
            DVLOG(2) << "checking heap after insert (size = " << m_size << ")";
            check_heap_condition();
        })
    }

private:
    inline void perlocate_down(size_t pos, len_t k) {
        auto lcp_k = (*m_array)[k];

        perlocation_dir_t dir = NONE;
        do {
            len_t lcp_lc = (lc(pos) < m_size) ? (*m_array)[m_heap[lc(pos)]] : 0;
            len_t lcp_rc = (rc(pos) < m_size) ? (*m_array)[m_heap[rc(pos)]] : 0;

            // find perlocation direction
            if(lcp_k < lcp_lc && lcp_k < lcp_rc) {
                // both children are larger, pick the largest
                dir = (lcp_lc > lcp_rc) ? LEFT : RIGHT;
            } else if(lcp_k < lcp_lc) {
                // go to the left
                dir = LEFT;
            } else if(lcp_k < lcp_rc) {
                // go to the right
                dir = RIGHT;
            } else {
                dir = NONE;
            }

            // go down if necessary
            if(dir == LEFT) {
                // left
                put(pos, m_heap[lc(pos)]);
                pos = lc(pos);
            } else if(dir == RIGHT) {
                // right
                put(pos, m_heap[rc(pos)]);
                pos = rc(pos);
            }
        } while(dir != NONE);

        put(pos, k);
    }

public:
    /// \brief Removes an item from the heap.
    ///
    /// \param i The index of the item in the key array. The key is retrieved
    ///          from there.
    inline void remove(len_t i) {
        auto pos = m_pos[i];
        if(pos != m_undef) { // never mind if it's not in the heap
            // get last element in heap
            auto k = m_heap[--m_size];

            // perlocate it down, starting at the former position of i
            perlocate_down(pos, k);

            m_pos[i] = m_undef; // i was removed
        }

        IF_PARANOID({
            DVLOG(2) << "checking heap after remove (size = " << m_size << ")";
            check_heap_condition();
        })
    }

    /// \brief Decreases the key of item in the heap.
    ///
    /// \tparam key_t the key type.
    /// \param i The index of the item in the key array. The key is retrieved
    ///          from there.
    /// \param value The new key value.
    template<typename key_t>
    inline void decrease_key(len_t i, key_t value) {
        (*m_array)[i] = value;

        auto pos = m_pos[i];
        DCHECK_NE(pos, m_undef) << "trying to decrease_key on an item that's not in the heap";

        if(pos != m_undef) {
            // perlocate item down, starting at its current position
            perlocate_down(pos, i);
        }

        IF_PARANOID({
            DVLOG(2) << "checking heap after decrease_key (size = " << m_size << ")";
            check_heap_condition();
        })
    }

    /// \brief Checks whether or not an item is contained in this heap.
    ///
    /// \param i The index of the item in the key array.
    /// \return \e true if the item is contained in the heap, \e false otherwise.
    inline bool contains(len_t i) const {
        return m_pos[i] != m_undef;
    }

    /// \brief Yields the number of items currently stored in the heap.
    /// \return The number of items currently stored in the heap.
    inline size_t size() const {
        return m_size;
    }

    /// \brief Gets the first (maximum) item from the heap.
    /// \return The index in the key array that points to the largest key.
    inline size_t get_max() const {
        return m_heap[0];
    }

    /// \brief Gets the top (first, maximum) item from the heap.
    /// \return The index in the key array that points to the largest key.
    inline size_t top() const {
        return get_max();
    }

    /// \brief Gets an item's key.
    ///
    /// \param i The index of the item in the key array.
    /// \return The item's key, retrieved from the key array.
    inline len_t key(len_t i) const {
        return (*m_array)[i];
    }

    // for tests?
    IF_DEBUG(
    inline void check_heap_condition() const {
        for(size_t i = 0; i < m_size; i++) {
            auto value = (*m_array)[m_heap[i]];
            if(lc(i) < m_size) DCHECK(value >= (*m_array)[m_heap[lc(i)]])
                << "heap condition violated for a left child";
            if(rc(i) < m_size) DCHECK(value >= (*m_array)[m_heap[rc(i)]])
                << "heap condition violated for a right child";
        }
    })
};

} //ns

