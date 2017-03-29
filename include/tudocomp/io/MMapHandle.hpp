#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <tudocomp/util/View.hpp>
#include <tudocomp/io/IOUtil.hpp>

namespace tdc {namespace io {
    inline size_t pagesize() {
        return sysconf(_SC_PAGESIZE);
    }

    /// A handle for a memory map.
    ///
    /// Can either be a file mapping or an anonymous mapping.
    class MMap {
        static constexpr const void* EMPTY = "";

        enum class State {
            Unmapped,
            Shared,
            Private
        };

        inline static size_t adj_size(size_t v) {
            return std::max(size_t(1), v);
        }

        inline static void check_mmap_error(void* ptr, string_ref descr) {
            if (ptr == MAP_FAILED) {
                perror("MMap error");
            }
            CHECK(ptr != MAP_FAILED) << "Error at " << descr;
        }
    public:
        enum class Mode {
            Read,
            ReadWrite
        };
    private:
        uint8_t* m_ptr   = (uint8_t*) EMPTY;
        size_t   m_size  = 0;

        State    m_state = State::Unmapped;
        Mode     m_mode  = Mode::Read;

    public:
        inline static bool is_offset_valid(size_t offset) {
            return (offset % pagesize()) == 0;
        }

        inline static size_t next_valid_offset(size_t offset) {
            auto ps = pagesize();
            auto diff = offset % ps;
            auto ok = offset - diff;
            DCHECK(is_offset_valid(ok));
            DCHECK(ok <= offset);
            return ok;
        }

        inline MMap() { /* field default values are fine already */ }

        inline MMap(const std::string& path,
             Mode mode,
             size_t size,
             size_t offset = 0)
        {
            m_mode = mode;
            m_size = size;

            DCHECK(is_offset_valid(offset))
                << "Offset must be page aligned, use MMap::next_valid_offset() to ensure this.";

            size_t file_size = read_file_size(path);
            bool needs_to_overallocate =
                (offset + m_size) > file_size;

            // Open file for memory map
            auto fd = open(path.c_str(), O_RDONLY);
            CHECK(fd != -1) << "Error at opening file";

            int mmap_prot;
            int mmap_flags;

            if (!needs_to_overallocate) {
                // Map file directly into memory

                if (m_mode == Mode::ReadWrite) {
                    mmap_prot = PROT_READ | PROT_WRITE;
                    mmap_flags = MAP_PRIVATE;
                    m_state = State::Private;
                } else {
                    mmap_prot = PROT_READ;
                    mmap_flags = MAP_SHARED;
                    m_state = State::Shared;
                }

                void* ptr = mmap(NULL,
                                adj_size(m_size),
                                mmap_prot,
                                mmap_flags,
                                fd,
                                offset);
                check_mmap_error(ptr, "mapping file into memory");

                m_ptr = (uint8_t*) ptr;
            } else {
                // Allocate memory and copy file into it

                *this = MMap(m_size);

                // seek to offset
                {
                    auto ret = lseek(fd, offset, SEEK_SET);
                    if (ret == -1) {
                        perror("Seeking fd");
                    }
                    CHECK(ret != -1);
                }

                // copy data
                {
                    auto ptr = m_ptr;
                    auto size = file_size - offset;

                    while (size > 0) {
                        auto ret = read(fd, ptr, size);
                        if (ret == -1) {
                            perror("Reading fd into mapped memory");
                        }
                        CHECK(ret >= 0);
                        size -= ret;
                        ptr += ret;
                    }

                }
            }
            close(fd);
        }

        inline MMap(size_t size)
        {
            m_mode = Mode::ReadWrite;
            m_size = size;

            int mmap_prot = PROT_READ | PROT_WRITE;
            int mmap_flags = MAP_PRIVATE | MAP_ANONYMOUS;

            void* ptr = mmap(NULL,
                             adj_size(m_size),
                             mmap_prot,
                             mmap_flags,
                             -1,
                             0);
            check_mmap_error(ptr, "creating anon. memory map");

            m_ptr = (uint8_t*) ptr;

            m_state = State::Private;
        }

        inline void remap(size_t new_size) {
            DCHECK(m_mode == Mode::ReadWrite);
            DCHECK(m_state == State::Private);

            auto p = mremap(m_ptr, adj_size(m_size), adj_size(new_size), MREMAP_MAYMOVE);
            std::cout << "old size: " << m_size << "\n";
            std::cout << "new size: " << new_size << "\n";
            check_mmap_error(p, "remapping memory");

            m_ptr = (uint8_t*) p;
            m_size =  new_size;
        }

        View view() const {
            return View(m_ptr, m_size);
        }

        GenericView<uint8_t> view() {
            const auto err = "Attempting to get a mutable view into a read-only mapping. Call the const overload of view() instead"_v;

            DCHECK(m_state == State::Private) << err;
            DCHECK(m_mode == Mode::ReadWrite) << err;
            return GenericView<uint8_t>(m_ptr, m_size);
        }

        inline MMap(const MMap& other) = delete;
        inline MMap& operator=(const MMap& other) = delete;
    private:
        inline void move_from(MMap&& other) {
            m_ptr   = other.m_ptr;
            m_size  = other.m_size;

            m_state = other.m_state;
            m_mode  = other.m_mode;

            other.m_state = State::Unmapped;
            other.m_ptr = (uint8_t*) EMPTY;
            other.m_size = 0;
        }
    public:
        inline MMap(MMap&& other) {
            move_from(std::move(other));
        }

        inline MMap& operator=(MMap&& other) {
            move_from(std::move(other));
            return *this;
        }

        ~MMap() {
            if (m_state != State::Unmapped) {
                DCHECK(m_ptr != EMPTY);

                int rc = munmap(m_ptr, adj_size(m_size));
                CHECK(rc == 0) << "Error at unmapping";
            }
        }
    };
}}
