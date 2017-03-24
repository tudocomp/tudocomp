#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <tudocomp/util/View.hpp>

namespace tdc {
namespace io {
    /*
        void *mmap(void *addr, size_t length, int prot, int flags,
                    int fd, off_t offset);
        int munmap(void *addr, size_t length);
        int open(const char *path, int oflags, mode_t mode);
        */

    class MMapHandle {
        View m_map;
        int m_fd;

        inline size_t get_filesize(const char* filename) {
            struct stat st;
            stat(filename, &st);
            return st.st_size;
        }

        inline void forget() {
            m_map = View((const uint8_t*) nullptr, 0);
            m_fd = -1;
        }
    public:
        inline MMapHandle(const std::string& path) {
            size_t filesize = get_filesize(path.c_str());
            //Open file
            int fd = open(path.c_str(), O_RDONLY);
            CHECK(fd != -1);
            //Execute mmap

            // TODO:  | MAP_POPULATE for pre-population
            //          MAP_PRIVATE  for ability to change

            void* mmappedData = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
            CHECK(mmappedData != MAP_FAILED);

            m_map = View((const uint8_t*) mmappedData, filesize);
            m_fd = fd;
        }

        inline MMapHandle(const MMapHandle& other) = delete;

        inline MMapHandle(MMapHandle&& other) {
            m_map = other.m_map;
            m_fd = other.m_fd;
            other.forget();
        }

        inline MMapHandle& operator=(MMapHandle&& other) {
            m_map = other.m_map;
            m_fd = other.m_fd;
            other.forget();
            return *this;
        }

        inline ~MMapHandle() {
            if (m_map.data() != nullptr) {
                int rc = munmap((void*) m_map.data(), m_map.size());
                CHECK(rc == 0);
                close(m_fd);
                forget();
            }
        }

        inline View view() {
            return m_map;
        }
    };
}
}
