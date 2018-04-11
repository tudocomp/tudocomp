#include <stdexcept>
#include <vector>

#include <tudocomp/def.hpp>

namespace tdc {
    /// \brief A ring buffer.
    ///
    /// A ring buffer provides a fixed-size buffer of elements that are aligned
    /// in a logical ring. When a new item is added when the buffer is full,
    /// the first item will be "rolled out" (i.e., removed) and the new item
    /// is appended to the end.
    ///
    /// \tparam T the item type
    template<typename T>
    class RingBuffer {
    private:
        std::vector<T> m_buffer;
        size_t m_start, m_end;
        bool m_empty;

        // simulates "i++", but also applies the modulo for the buffer size
        inline size_t post_inc(size_t& i) const {
            const size_t r = i;
            i = (i+1) % m_buffer.size();
            return r;
        }

    public:
        /// \brief Constructor.
        ///
        /// \param bufsize the ring buffer size
        inline RingBuffer(size_t bufsize)
            : m_start(0), m_end(0), m_empty(true) 
        {
            m_buffer.resize(bufsize);
        }

        /// \brief Placed a new item at the end of the buffer.
        ///
        /// In case the buffer is full, the first item will be removed.
        ///
        /// \param item the item to push
        inline void push_back(const T& item) {
            if(m_end == m_start && !m_empty) {
                post_inc(m_start); // remove head
            }

            // insert item
            m_buffer[post_inc(m_end)] = item;

            // update empty flag
            m_empty = false;
        }

        /// \brief Gets and removes the item at the start of the buffer.
        inline T pop_front() {
            if(tdc_unlikely(m_empty)) {
                // fail if empty
                throw std::runtime_error(
                    "attempt to pop from an empty ring buffer");
            } else {
                // remove first
                auto item = m_buffer[post_inc(m_start)];

                // update empty flag and return
                m_empty = (m_start == m_end);
                return item;
            }
        }

        /// \brief Tells whether the buffer is empty.
        inline bool empty() const {
            return m_empty;
        }

        /// \brief Gets the ring buffer's capacity.
        inline size_t capacity() const {
            return m_buffer.size();
        }

        /// \brief Gets the ring buffer's current amount of entries.
        inline size_t size() const {
            if(m_start == m_end) {
                return m_empty ? 0 : capacity();
            } else if(m_start < m_end) {
                return m_end - m_start;
            } else { // m_first > m_last
                return (capacity() - m_start + 1) + m_end; //TODO test
            }
        }

        /// \brief Constant access iterator.
        class const_iterator {
        protected:
            const RingBuffer* m_rb;
            size_t m_pos;
            bool m_end;

        public:
            inline const_iterator(const RingBuffer& rb, bool end = false) :
                m_rb(&rb), m_pos(rb.m_start), m_end(end || rb.m_empty)
            {
            }

            const_iterator& operator++() {
                if(!m_end) {
                    m_rb->post_inc(m_pos);
                    m_end = (m_pos == m_rb->m_end);
                }
                return *this;
            }

            const_iterator operator++(int) {
                auto r = *this; ++(*this); return r;
            }

            bool operator==(const_iterator other) const {
                return m_rb == other.m_rb && (
                            m_end == other.m_end ||
                            m_pos == other.m_pos);
            }

            bool operator!=(const_iterator other) const {
                return !(*this == other);
            }

            const T& operator*() const {
                if(tdc_unlikely(m_end)) {
                    throw std::runtime_error("iterator reached end");
                } else {
                    return m_rb->m_buffer[m_pos];
                }
            }
        };

        const_iterator begin() { return const_iterator(*this); }
        const_iterator end()   { return const_iterator(*this, true); }

        const_iterator cbegin() { return begin(); }
        const_iterator cend()   { return end(); }
    };
}

