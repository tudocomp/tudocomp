#ifndef _INCLUDED_IOUTIL_HPP
#define _INCLUDED_IOUTIL_HPP

#include <fstream>
#include <iostream>
#include <string>

namespace tdc {
namespace io {

/// \cond INTERNAL

// TODO: Error handling

template<class T>
T read_bytes(std::istream& inp, size_t bytes = sizeof(T)) {
    char c;
    T ret = 0;
    for(size_t i = 0; i < bytes; i++) {
        if (!inp.get(c)) {
            break;
        }
        ret <<= 8;
        ret |= T((unsigned char)c);
    }
    return ret;
}

template<class T>
void read_bytes_to_vec(std::istream& inp, T& vec, size_t bytes) {
    char c;
    for(size_t i = 0; i < bytes; i++) {
        if (!inp.get(c)) {
            break;
        }
        vec.push_back((unsigned char)c);
    }
}

template<class T>
T read_file_to_stl_byte_container(std::string& filename,
                                  size_t offset = 0,
                                  bool null_term = false) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        T contents;

        // first, determine length from offset to end of file
        in.seekg(offset, std::ios::beg);
        auto start = in.tellg();
        in.seekg(0, std::ios::end);
        auto end = in.tellg();
        auto len = end - start;

        // use length to allocate a single buffer for the file
        if (null_term) {
            contents.resize(len + 1, 0);
        } else {
            contents.resize(len);
        }

        // set position back to the start position at offset
        in.seekg(offset, std::ios::beg);

        // read file into contents without reallocating
        if (null_term) {
            in.read((char*)&contents[0], contents.size() - 1);
        } else {
            in.read((char*)&contents[0], contents.size());
        }
        in.close();
        return(contents);
    }
    throw(errno);
}

template<class T, class S>
T read_stream_to_stl_byte_container(S& stream) {
    T vector;
    char c;
    while (stream.get(c)) {
        vector.push_back(typename T::value_type(c));
    }
    return(vector);
}

inline size_t read_file_size(std::string& file) {
    std::ifstream t(file);
    if (t) {
        t.seekg(0, std::ios::end);
        return t.tellg();
    }
    throw(errno);
}

/// \endcond

}
}

#endif
