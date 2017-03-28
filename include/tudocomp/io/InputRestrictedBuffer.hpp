#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <tudocomp/io/MMapHandle.hpp>
#include <tudocomp/io/InputSource.hpp>
#include <tudocomp/io/InputRestrictions.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/EscapeMap.hpp>

namespace tdc {namespace io {
    class RestrictedBuffer {
    public:
        static const size_t npos = -1;
    private:
        MMap m_map;
        View m_restricted_data;

        // Both relative to entire original input data:
        size_t                m_from = 0;
        size_t                m_to = npos;

        io::InputRestrictions m_restrictions;
        InputSource m_source;

        size_t m_unrestricted_size = 0;

        // mmap needs to be page aligned, so for file mappings
        // we need to store a offset
        //
        // Effectivley this means that for a given mmap in instance in this class,
        // instead of
        // |-----------input--------------|
        // [          [from_______to]     ]
        // it stores
        // |-----------input--------------|
        // [   [offset|from_______to]     ]
        size_t m_mmap_page_offset = 0;

        template<typename I, typename J>
        inline void escape_with_iters(I read_begin, I read_end, J write_end, bool do_copy = false) {
            if (!m_restrictions.has_no_escape_restrictions()) {
                FastEscapeMap fast_escape_map;
                uint8_t escape_byte;

                {
                    EscapeMap em(m_restrictions);
                    fast_escape_map = FastEscapeMap(em);
                    escape_byte = fast_escape_map.escape_byte();
                }

                while(read_begin != read_end) {
                    --read_end;
                    --write_end;

                    uint8_t current_byte = *read_end;

                    *write_end = fast_escape_map.lookup_byte(current_byte);

                    if (fast_escape_map.lookup_flag_bool(current_byte)) {
                        --write_end;
                        *write_end = escape_byte;
                    }
                }
            } else if (do_copy) {
                while(read_begin != read_end) {
                    --read_end;
                    --write_end;
                    *write_end = *read_end;
                }
            }
        }

        // NB: The len argument would be redundant, but exists because
        // the istream iterator does not allow efficient slicing of the input file
        // TODO: Define custom sliceable ifstream iterator
        // TODO: ^ Needed in Input as well
        template<typename T>
        inline size_t extra_size_needed_due_restrictions(T begin, T end, size_t len) {
            size_t extra = 0;

            if (!m_restrictions.has_no_escape_restrictions()) {
                size_t i = 0;

                FastEscapeMap fast_escape_map{EscapeMap(m_restrictions)};

                while((begin != end) && (i < len)) {
                    uint8_t current_byte = *begin;

                    extra += fast_escape_map.lookup_flag(current_byte);

                    ++begin;
                    ++i;
                }

                DCHECK_EQ(i, len);
            }

            if (m_restrictions.null_terminate()) {
                extra++;
            }

            return extra;
        }

        inline void init() {
            if (m_source.is_view()) {
                View s;
                if (m_to == npos) {
                    s = m_source.view().slice(m_from);
                } else {
                    s = m_source.view().slice(m_from, m_to);
                }

                m_unrestricted_size = s.size();

                size_t extra_size = extra_size_needed_due_restrictions(
                    s.cbegin(), s.cend(), s.size());

                if (extra_size != 0) {
                    size_t size = s.size() + extra_size;
                    m_map = MMap(size);

                    {
                        GenericView<uint8_t> target = m_map.view();
                        size_t noff = m_restrictions.null_terminate()? 1 : 0;
                        escape_with_iters(s.cbegin(), s.cend(), target.end() - noff, true);
                        // For null termination, a trailing byte is implicit 0
                    }

                    m_restricted_data = m_map.view();
                } else {
                    m_restricted_data = s;
                }
            } else if (m_source.is_file()) {
                // iterate file to check for escapeable bytes and also null

                if (m_to == npos) {
                    m_unrestricted_size = read_file_size(m_source.file()) - m_from;
                } else {
                    m_unrestricted_size = m_to - m_from;
                }

                auto path = m_source.file();
                auto c_path = path.c_str();

                size_t extra_size = 0;
                {
                    auto ifs = create_tdc_ifstream(c_path, m_from);

                    std::istream_iterator<char> begin (ifs);
                    std::istream_iterator<char> end;
                    extra_size = extra_size_needed_due_restrictions(
                        begin, end, m_unrestricted_size);
                }

                size_t aligned_offset = MMap::next_valid_offset(m_from);
                m_mmap_page_offset = m_from - aligned_offset;

                DCHECK_EQ(aligned_offset + m_mmap_page_offset, m_from);

                size_t map_size = m_unrestricted_size + extra_size + m_mmap_page_offset;

                if (m_restrictions.has_no_restrictions()) {
                    m_map = MMap(path, MMap::Mode::Read, map_size, aligned_offset);

                    const auto& m = m_map;
                    m_restricted_data = m.view().slice(m_mmap_page_offset);
                } else {
                    m_map = MMap(path, MMap::Mode::ReadWrite, map_size, aligned_offset);

                    size_t noff = m_restrictions.null_terminate()? 1 : 0;

                    uint8_t* begin_file_data = m_map.view().begin() + m_mmap_page_offset;
                    uint8_t* end_file_data   = begin_file_data      + m_unrestricted_size;
                    uint8_t* end_data        = end_file_data        + extra_size - noff;
                    escape_with_iters(begin_file_data, end_file_data, end_data);
                    if (m_restrictions.null_terminate() && (*end_data != 0)) {
                        // ensure the last valid byte is actually 0 if using null termination
                        *end_data = 0;
                    }
                    m_restricted_data = m_map.view().slice(m_mmap_page_offset);
                }
            } else if (m_source.is_stream()) {
                DCHECK_EQ(m_from, 0);
                DCHECK_EQ(m_to, npos);

                // Start with a typical page size to not realloc as often
                // for small inputs
                size_t capacity = pagesize();
                size_t size = 0;
                size_t extra_size = 0;
                FastEscapeMap fast_escape_map;
                if (!m_restrictions.has_no_escape_restrictions()) {
                    fast_escape_map = FastEscapeMap {
                        EscapeMap(m_restrictions)
                    };
                }

                size_t noff = m_restrictions.null_terminate()? 1 : 0;
                extra_size += noff;

                // Initial allocation

                m_map = MMap(capacity);

                // Fill and grow
                {
                    std::istream& is = *(m_source.stream());
                    bool done = false;

                    while(!done) {
                        // fill until capacity
                        uint8_t* ptr = m_map.view().begin() + size;
                        while(size < capacity) {
                            char c;
                            if(!is.get(c)) {
                                done = true;
                                break;
                            } else {
                                *ptr = uint8_t(c);
                                ++ptr;
                                ++size;
                                extra_size += fast_escape_map.lookup_flag(uint8_t(c));
                            }
                        }
                        if (done) break;

                        // realloc to greater size;
                        capacity *= 2;
                        m_map.remap(capacity);
                    }

                    // Throw away overallocation
                    std::cout << "Size, extra size: " << size << ", " << extra_size << "\n";
                    // For null termination,
                    // a trailing unwritten byte is automatically 0
                    m_map.remap(size + extra_size);

                    m_unrestricted_size = size;
                    m_restricted_data = m_map.view();
                }

                // Escape
                {
                    uint8_t* begin_stream_data = m_map.view().begin();
                    uint8_t* end_stream_data   = begin_stream_data + size;
                    uint8_t* end_data          = end_stream_data   + extra_size - noff;
                    escape_with_iters(begin_stream_data, end_stream_data, end_data);
                }
            } else {
                DCHECK(false) << "This should not happen";
            }
        }

        // Change the restrictions on a stream restricted buffer
        inline static RestrictedBuffer unrestrict(RestrictedBuffer&& other) {
            if (other.restrictions().has_no_restrictions()) {
                return std::move(other);
            }
            DCHECK(other.restrictions().has_restrictions());
            DCHECK(other.source().is_stream());
            DCHECK(other.m_mmap_page_offset == 0);

            auto x = std::move(other);

            auto r = x.m_restrictions;

            std::cout << "Old restrictions: " << r << "\n";
            auto start = x.m_map.view().begin();
            auto end   = x.m_map.view().end();

            FastUnescapeMap fast_unescape_map { EscapeMap(r) };

            auto read_p = start;
            auto write_p = start;

            size_t debug_counter = 0;

            size_t noff = x.m_restrictions.null_terminate()? 1 : 0;

            auto data_end = end - noff;

            while (read_p != data_end) {
                if (*read_p == fast_unescape_map.escape_byte()) {
                    ++read_p;
                    *write_p = fast_unescape_map.lookup_byte(*read_p);
                } else {
                    *write_p = *read_p;
                }
                ++read_p;
                ++write_p;

                ++debug_counter;
            }

            DCHECK_EQ(debug_counter, x.m_unrestricted_size);

            auto old_size = x.m_map.view().size();
            auto reduced_size = (read_p - write_p) + noff;

            x.m_map.remap(old_size - reduced_size);
            x.m_restrictions = InputRestrictions();
            x.m_restricted_data = x.m_map.view();

            return x;
        }

        // Change the restrictions on a stream restricted buffer
        inline static RestrictedBuffer restrict(RestrictedBuffer&& other,
                                                const io::InputRestrictions& restrictions) {
            if (other.restrictions() == restrictions) {
                return std::move(other);
            }
            DCHECK(other.restrictions().has_no_restrictions());
            DCHECK(other.source().is_stream());
            DCHECK(other.m_mmap_page_offset == 0);

            size_t old_size;
            size_t extra_size;

            // Calculate needed extra size:
            {
                View s = other.view();
                other.m_restrictions = restrictions;
                extra_size = other.extra_size_needed_due_restrictions(
                    s.cbegin(), s.cend(), s.size()
                );
                old_size = s.size();
            }

            // If nothing about the actual data changed
            // return it as is
            if (extra_size == 0) {
                return std::move(other);
            }

            // Else remap and expand the data by escaping:

            other.m_map.remap(old_size + extra_size);
            other.m_restricted_data = other.m_map.view();

            {
                size_t noff = other.m_restrictions.null_terminate()? 1 : 0;

                uint8_t* start = other.m_map.view().begin();
                uint8_t* old_end = start + old_size;
                uint8_t* new_end = start + old_size + extra_size - noff;

                other.escape_with_iters(start, old_end, new_end);
                if (other.m_restrictions.null_terminate()) {
                    *new_end = 0;
                }

                DCHECK_EQ(new_end + 1, other.m_map.view().end());
            }

            return std::move(other);
        }

    public:
        inline static RestrictedBuffer change_restrictions(
            RestrictedBuffer&& other,
            const io::InputRestrictions& restrictions)
        {
            auto buf = unrestrict(std::move(other));
            return restrict(std::move(buf), restrictions);
        }

        inline RestrictedBuffer(const InputSource& src,
                                size_t from,
                                size_t to,
                                io::InputRestrictions restrictions):
            m_from(from),
            m_to(to),
            m_restrictions(restrictions),
            m_source(src)
        {
            init();
        }

        inline size_t from() const { return m_from; }
        inline size_t to() const { return m_to; }
        inline const InputRestrictions& restrictions() const { return m_restrictions; }
        inline const InputSource& source() const { return m_source; }

        inline size_t unrestricted_size() const { return m_unrestricted_size; }
        inline View view() const { return m_restricted_data; }

        inline RestrictedBuffer() = delete;
    };

}}
