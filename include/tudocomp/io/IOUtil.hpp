#pragma once

#include <fstream>
#include <iostream>
#include <string>

namespace tdc {
namespace io {

/// \cond INTERNAL

// TODO: Error handling

inline std::runtime_error tdc_input_file_not_found_error(const std::string& path) {
    return std::runtime_error(std::string("input file ") + path + " does not exist");
}

inline std::runtime_error tdc_output_file_not_found_error(const std::string& path) {
    return std::runtime_error(std::string("output file ") + path + " can not be created/accessed");
}

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
T read_file_to_stl_byte_container(const std::string& filename,
                                  size_t offset = 0) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (bool(in)) {
        T contents;

        // first, determine length from offset to end of file
        in.seekg(offset, std::ios::beg);
        auto start = in.tellg();
        in.seekg(0, std::ios::end);
        auto end = in.tellg();
        auto len = end - start;

        // use length to allocate a single buffer for the file
        contents.resize(len);

        // set position back to the start position at offset
        in.seekg(offset, std::ios::beg);

        // read file into contents without reallocating
        in.read((char*)&contents[0], contents.size());
        in.close();
        return(contents);
    }
    throw tdc_input_file_not_found_error(filename);
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

inline size_t read_file_size(const std::string& file) {
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

