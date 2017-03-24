#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <tudocomp/io/InputSource.hpp>
#include <tudocomp/io/InputRestrictions.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/EscapeMap.hpp>

namespace tdc {namespace io {
    class RestrictedBufferSourceKeepalive {
    public:
        virtual ~RestrictedBufferSourceKeepalive() {}
    };

    inline size_t pagesize() {
        return sysconf(_SC_PAGESIZE);
    }

    class RestrictedBuffer {
    public:
        static const size_t npos = -1;
    private:

        enum class State {
            NotOwned,
            Shared,
            Private
        };

        State    m_mmap_state = State::NotOwned;
        uint8_t* m_mmap_ptr   = nullptr;
        size_t   m_mmap_size  = 0;
        int      m_mmap_fd    = -1;

        // Both relative to entire original input data:
        size_t                m_from = 0;
        size_t                m_to = npos;

        io::InputRestrictions m_restrictions;
        InputSource m_source;

        size_t m_unrestricted_size = 0;

        // mmap needs to be page aligned, so for file mappings
        // we need to store a offset
        size_t m_mmap_page_offset = 0;

        std::shared_ptr<RestrictedBufferSourceKeepalive> m_keepalive;

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
            if (m_source.is_stream()) {
                DCHECK_EQ(m_from, 0);
            }

            if (m_source.is_view()) {
                View s;
                if (m_to == npos) {
                    s = m_source.view().slice(m_from);
                } else {
                    s = m_source.view().slice(m_from, m_to);
                }

                size_t extra_size = extra_size_needed_due_restrictions(
                    s.cbegin(), s.cend(), s.size());

                if (extra_size != 0) {
                    int mmap_prot = PROT_READ | PROT_WRITE;;
                    int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;;

                    size_t size = s.size() + extra_size;

                    void* mmappedData = mmap(NULL, size, mmap_prot, mmap_flags, -1, 0);
                    CHECK(mmappedData != MAP_FAILED) << "Error at mapping memory";

                    GenericView<uint8_t> target((uint8_t*) mmappedData, size);

                    size_t noff = m_restrictions.null_terminate()? 1 : 0;

                    escape_with_iters(s.cbegin(), s.cend(), target.end() - noff, true);

                    m_mmap_state = State::Private;
                    m_mmap_ptr = (uint8_t*) mmappedData;
                    m_mmap_size = size;
                    m_mmap_fd = -1;
                    m_unrestricted_size = s.size();
                } else {
                    m_mmap_state = State::NotOwned;
                    m_mmap_ptr   = (uint8_t*) s.data();
                    m_mmap_size  = s.size();
                    m_mmap_fd    = -1;
                    m_unrestricted_size = s.size();
                }
            } else if (m_source.is_file()) {
                // iterate file to check for escapeable bytes and also null

                size_t original_size;
                if (m_to == npos) {
                    original_size = read_file_size(m_source.file()) - m_from;
                } else {
                    original_size = m_to - m_from;
                }

                auto c_path = m_source.file().c_str();

                size_t extra_size = 0;
                {
                    auto ifs = create_tdc_ifstream(c_path, m_from);

                    std::istream_iterator<char> begin (ifs);
                    std::istream_iterator<char> end;
                    extra_size = extra_size_needed_due_restrictions(
                        begin, end, original_size);
                }

                // Open file for memory map
                int fd = open(c_path, O_RDONLY);

                CHECK(fd != -1) << "Error at opening file";

                // Map into memory

                int mmap_prot;
                int mmap_flags;
                State mmap_state;
                bool needs_escaping = false;

                m_unrestricted_size = original_size;

                if (extra_size == 1 && m_restrictions.null_terminate()) {
                    // Null termination happens by adding the implicit 0 at the end
                    mmap_prot = PROT_READ;
                    mmap_flags = MAP_SHARED;
                    mmap_state = State::Shared;
                } else if (extra_size == 0) {
                    mmap_prot = PROT_READ;
                    mmap_flags = MAP_SHARED;
                    mmap_state = State::Shared;
                } else {
                    mmap_prot = PROT_READ | PROT_WRITE;
                    mmap_flags = MAP_PRIVATE;
                    mmap_state = State::Private;
                    needs_escaping = true;
                }

                size_t aligned_m_from = (m_from / pagesize()) * pagesize();
                m_mmap_page_offset = m_from % pagesize();
                DCHECK_EQ(aligned_m_from + m_mmap_page_offset, m_from);

                void* mmappedData = mmap(NULL,
                                         original_size + extra_size + m_mmap_page_offset,
                                         mmap_prot,
                                         mmap_flags,
                                         fd,
                                         aligned_m_from);
                CHECK(mmappedData != MAP_FAILED) << "Error at mapping file into memory";

                if (needs_escaping) {
                    size_t noff = m_restrictions.null_terminate()? 1 : 0;

                    uint8_t* begin_file_data = (uint8_t*) mmappedData + m_mmap_page_offset;
                    uint8_t* end_file_data = begin_file_data + original_size;
                    uint8_t* end_alloc = begin_file_data + original_size + extra_size - noff;
                    escape_with_iters(begin_file_data, end_file_data, end_alloc);
                }

                m_mmap_state = mmap_state;
                m_mmap_ptr = (uint8_t*) mmappedData;
                m_mmap_size = original_size + extra_size + m_mmap_page_offset;
                m_mmap_fd = fd;
            } else if (m_source.is_stream()) {
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

                int mmap_prot = PROT_READ | PROT_WRITE;;
                int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;;

                void* mmappedData = mmap(NULL, capacity, mmap_prot, mmap_flags, -1, 0);
                CHECK(mmappedData != MAP_FAILED) << "Error at mapping memory";

                // Fill and grow
                {
                    std::istream& is = *(m_source.stream());
                    bool done = false;

                    while(!done) {
                        // fill until capacity
                        uint8_t* ptr = ((uint8_t*) mmappedData) + size;
                        while(size < capacity) {
                            char c;
                            // TODO: Direct buffer copy here
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
                        auto old_capacity = capacity;
                        capacity *= 2;

                        mmappedData = mremap(mmappedData, old_capacity, capacity, MREMAP_MAYMOVE);
                        CHECK(mmappedData != MAP_FAILED) << "Error at remapping memory";
                    }

                    mmappedData = mremap(mmappedData, capacity, size + extra_size, MREMAP_MAYMOVE);
                    CHECK(mmappedData != MAP_FAILED) << "Error at remapping memory";
                }

                // Escape
                {
                    uint8_t* begin_stream_data = (uint8_t*) mmappedData;
                    uint8_t* end_stream_data = begin_stream_data + size;
                    uint8_t* end_alloc = begin_stream_data + size + extra_size - noff;
                    escape_with_iters(begin_stream_data, end_stream_data, end_alloc);
                }

                m_mmap_state = State::Private;
                m_mmap_ptr = (uint8_t*) mmappedData;
                m_mmap_size = size + extra_size;
                m_mmap_fd = -1;

                m_unrestricted_size = size;
            } else {
                DCHECK(false) << "This should not happen";
            }
        }

        inline static RestrictedBuffer unrestrict(RestrictedBuffer&& other) {
            auto x = std::move(other);
            DCHECK(x.m_mmap_fd == -1);
            DCHECK(x.m_mmap_state == State::Private);

            auto r = x.m_restrictions;

            auto restricted_data_offset = x.m_mmap_page_offset;

            auto start = x.m_mmap_ptr;
            auto end = x.m_mmap_ptr + x.m_mmap_size;

            FastUnescapeMap fast_unescape_map { EscapeMap(r) };

            auto read_p = start + restricted_data_offset;
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

            auto reduced_size = (read_p - write_p) + noff;

            auto mmappedData = mremap(x.m_mmap_ptr,
                                      x.m_mmap_size,
                                      x.m_mmap_size - reduced_size,
                                      MREMAP_MAYMOVE);

            if (mmappedData == MAP_FAILED) {
                std::cout << "old size: " << x.m_mmap_size << "\n";
                std::cout << "new size: " << x.m_mmap_size - reduced_size << "\n";
            }
            CHECK(mmappedData != MAP_FAILED) << "Error at remapping memory";

            x.m_mmap_ptr = (uint8_t*) mmappedData;
            x.m_mmap_size = x.m_mmap_size - reduced_size;
            x.m_restrictions = InputRestrictions();

            return x;
        }

    public:
        inline static RestrictedBuffer restrict(RestrictedBuffer&& other,
                                                size_t from,
                                                size_t to,
                                                const io::InputRestrictions& restrictions) {
            // This function currently only works under these conditions:
            DCHECK(other.m_from <= from);
            DCHECK(to <= other.m_to);

            if (other.m_restrictions.has_restrictions()) {
                other = unrestrict(std::move(other));

                if (restrictions == other.m_restrictions) {
                    return std::move(other);
                }
            }

            DCHECK(!(restrictions == other.m_restrictions));
            DCHECK(other.m_restrictions.has_no_restrictions());

            if (other.m_source.is_stream()) {
                // Special path

                // Allocation is owned already, and has no
                // restrictions on the data.
                // Need to find out extra size and remap.

                // NB: We keep the original prefix of the data around
                // as an optimization for the algorithm
                // that does the escaping.
                // This can be changed in case this ever becomes a performance
                // concern.

                View s = other.view();

                // Existing allocation:
                // [page offset|from...to]

                // Figure out new slice sizes:
                auto from_diff = from - other.from();
                size_t len = 0;
                if (to == npos) {
                    len = s.size() - from_diff;
                } else {
                    len = to - from;
                }

                s = s.substr(from_diff, len);

                other.m_restrictions = restrictions;

                // Extra size of subslice:
                size_t extra_size = other.extra_size_needed_due_restrictions(
                    s.cbegin(), s.cend(), s.size()
                );

                auto discarded_suffix = other.view().size() - (from_diff + len);

                size_t new_mm_size = 0;
                if (discarded_suffix < extra_size) {
                    new_mm_size = other.m_mmap_size + (extra_size - discarded_suffix);
                } else {
                    new_mm_size = other.m_mmap_size - (discarded_suffix - extra_size);
                }

                auto mmappedData = mremap(other.m_mmap_ptr,
                                          other.m_mmap_size,
                                          new_mm_size,
                                          MREMAP_MAYMOVE);
                if (mmappedData == MAP_FAILED) {
                    std::cout << "old size: " << other.m_mmap_size << "\n";
                    std::cout << "new size: " << new_mm_size << "\n";
                }
                CHECK(mmappedData != MAP_FAILED) << "Error at remapping memory";

                other.m_mmap_ptr = (uint8_t*) mmappedData;
                other.m_mmap_size = new_mm_size;

                other.m_from = from;
                other.m_to = to;

                other.m_unrestricted_size = len;
                other.m_mmap_page_offset = from_diff;

                {
                    size_t noff = other.m_restrictions.null_terminate()? 1 : 0;

                    uint8_t* start = other.m_mmap_ptr + from_diff;
                    uint8_t* old_end = start + len;
                    uint8_t* new_end = start + len + extra_size - noff;

                    other.escape_with_iters(start, old_end, new_end);
                    if (other.m_restrictions.null_terminate()) {
                        new_end[1] = 0;
                    }

                    DCHECK_EQ(new_end + 1, other.m_mmap_ptr + other.m_mmap_size);
                }

                return std::move(other);
            } else { // if (other.m_source.is_file() || other.m_source.is_view()) {
                // Normal path, because reusing the allocation
                // will be similary costly to recreating it
                auto src = other.m_source;
                {
                    auto dropme = std::move(other);
                }
                return RestrictedBuffer(src, from, to, restrictions);
            }
        }

        inline ~RestrictedBuffer() {
            if (m_mmap_state != State::NotOwned) {
                DCHECK(m_mmap_ptr != nullptr);

                int rc = munmap(m_mmap_ptr, m_mmap_size);
                CHECK(rc == 0);

                if (m_mmap_fd != -1) {
                    close(m_mmap_fd);
                    m_mmap_fd = -1;
                }
            } else {
                DCHECK(m_mmap_fd == -1);
            }
        }

        inline RestrictedBuffer(const InputSource& src,
                                size_t from,
                                size_t to,
                                io::InputRestrictions restrictions,
                                std::shared_ptr<RestrictedBufferSourceKeepalive> keepalive
                                  = std::shared_ptr<RestrictedBufferSourceKeepalive>()):
            m_from(from),
            m_to(to),
            m_restrictions(restrictions),
            m_source(src),
            m_keepalive(keepalive)
        {
            init();
        }

        inline size_t from() const { return m_from; }
        inline size_t to() const { return m_to; }
        inline const InputRestrictions& restrictions() const { return m_restrictions; }
        inline const InputSource& source() const { return m_source; }

        inline size_t size() const { return m_mmap_size - m_mmap_page_offset; }
        inline size_t unrestricted_size() const { return m_unrestricted_size; }
        inline View view() const {
            return View(m_mmap_ptr + m_mmap_page_offset, m_mmap_size - m_mmap_page_offset);
        }

        inline RestrictedBuffer() = delete;
        inline RestrictedBuffer(const RestrictedBuffer& other) = delete;
        inline RestrictedBuffer& operator=(const RestrictedBuffer& other) = delete;
    private:
        inline void move_from(RestrictedBuffer&& other) {
            m_mmap_state = other.m_mmap_state;
            m_mmap_ptr = other.m_mmap_ptr;
            m_mmap_size = other.m_mmap_size;
            m_mmap_fd = other.m_mmap_fd;

            m_from = other.m_from;
            m_to = other.m_to;
            m_restrictions = std::move(other.m_restrictions);
            m_source = std::move(other.m_source);

            m_unrestricted_size = other.m_unrestricted_size;

            m_mmap_page_offset = other.m_mmap_page_offset;

            other.m_mmap_state = State::NotOwned;
            other.m_mmap_fd = -1;
        }
    public:
        inline RestrictedBuffer(RestrictedBuffer&& other):
            m_source(""_v) // hack
        {
            move_from(std::move(other));
        }

        inline RestrictedBuffer& operator=(RestrictedBuffer&& other) {
            move_from(std::move(other));
            return *this;
        }
    };

}}
