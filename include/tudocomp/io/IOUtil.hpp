#pragma once

#include <climits>
#include <fstream>
#include <iostream>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

inline std::ifstream create_tdc_ifstream(const std::string& filename, size_t offset = 0) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (bool(in)) {
        in.seekg(offset, std::ios::beg);
        return in;
    }
    throw tdc_input_file_not_found_error(filename);
}

template<class T>
T read_file_to_stl_byte_container(const std::string& filename,
                                  size_t offset = 0,
                                  size_t len = -1) {
    auto in = create_tdc_ifstream(filename, offset);
    T contents;

    // if needed, determine length from offset to end of file
    if (len == size_t(-1)) {
        in.seekg(offset, std::ios::beg);
        auto start = in.tellg();
        in.seekg(0, std::ios::end);
        auto end = in.tellg();
        len = end - start;
    }

    // use length to allocate a single buffer for the file
    contents.reserve(len);
    contents.resize(len);

    // set position back to the start position at offset
    in.seekg(offset, std::ios::beg);

    // read file into contents without reallocating
    in.read((char*)&contents[0], contents.size());
    in.close();

    return(contents);
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
    // TODO: Maybe replace by calling stat
    auto in  = create_tdc_ifstream(file);
    in.seekg(0, std::ios::end);

    return in.tellg();
}

/// \endcond

inline bool file_exists(const std::string& filename) {
    auto path = filename.c_str();
    struct stat path_stat;

    bool does_exist = (stat(path, &path_stat) == 0);

    // Check if it exists, and if its also a regular file
    return does_exist && S_ISREG(path_stat.st_mode);
}

template<class T, typename BitSink>
inline void write_int(BitSink&& sink, T value, size_t bits = sizeof(T) * CHAR_BIT) {
    for (int i = bits - 1; i >= 0; i--) {
        sink.write_bit((value & T(T(1) << i)) != T(0));
    }
}

template<class T, typename BitSink>
inline T read_int(BitSink&& sink, size_t amount = sizeof(T) * CHAR_BIT) {
    T value = 0;
    for(size_t i = 0; i < amount; i++) {
        value <<= 1;
        value |= sink.read_bit();
    }
    return value;
}

template<typename T, typename BitSink>
inline void write_compressed_int(BitSink&& sink, T v, size_t b = 7) {
    DCHECK(b > 0);

    uint64_t u = uint64_t(v);
    uint64_t mask = (u << b) - 1;
    do {
        uint64_t current = v & mask;
        v >>= b;

        sink.write_bit(v > 0);
        sink.write_int(current, b);
    } while(v > 0);
}

template<typename T, typename BitSink>
inline T read_compressed_int(BitSink&& sink, size_t b = 7) {
    DCHECK(b > 0);

    uint64_t value = 0;
    size_t i = 0;

    bool has_next;
    do {
        has_next = sink.read_bit();
        value |= (sink.template read_int<size_t>(b) << (b * (i++)));
    } while(has_next);

    return T(value);
}

template<typename T, typename BitSink>
inline void write_unary(BitSink&& sink, T v) {
    while(v--) {
        sink.write_bit(0);
    }

    sink.write_bit(1);
}

template<typename T, typename BitSink>
inline T read_unary(BitSink&& sink) {
    T v = 0;
    while(!sink.read_bit()) ++v;
    return v;
}

template<typename T, typename BitSink>
inline void write_ternary(BitSink&& sink, T v) {
    if(v) {
        --v;
        do {
            sink.write_int(v % 3, 2); // 0 -> 00, 1 -> 01, 2 -> 10
            v /= 3;
        } while(v);
    }
    sink.write_int(3, 2); // terminator -> 11
}

template<typename T, typename BitSink>
inline T read_ternary(BitSink&& sink) {
    size_t mod = sink.template read_int<size_t>(2);
    T v = 0;
    if(mod < 3) {
        size_t b = 1;
        do {
            v += mod * b;
            b *= 3;
            mod = sink.template read_int<size_t>(2);
        } while(mod != 3);

        ++v;
    }
    return v;
}

template<typename T, typename BitSink>
inline void write_elias_gamma(BitSink&& sink, T v) {
    DCHECK_GT(v, 0) << "zero cannot be gamma-encoded";

    const auto m = bits_for(v) - 1;
    write_unary(sink, m);
    if(m > 0) sink.write_int(v, m); // cut off leading 1
}

template<typename T, typename BitSink>
inline T read_elias_gamma(BitSink&& sink) {
    auto m = read_unary<size_t>(sink);
    if(m > 0) {
        return T((1ULL << m) | sink.template read_int<uint64_t>(m));
    } else {
        return T(1);
    }
}

template<typename T, typename BitSink>
inline void write_elias_delta(BitSink&& sink, T v) {
    DCHECK_GT(v, 0) << "zero cannot be delta-encoded";

    auto m = bits_for(v) - 1;
    write_elias_gamma(sink, m+1);
    if(m > 0) sink.write_int(v, m); // cut off leading 1
}

template<typename T, typename BitSink>
inline T read_elias_delta(BitSink&& sink) {
    auto m = read_elias_gamma<size_t>(sink) - 1;
    if(m > 0) {
        return T((1ULL << m) | sink.template read_int<uint64_t>(m));
    } else {
        return T(1);
    }
}

template<typename T, typename BitSink>
inline void write_rice(BitSink&& sink, T v, uint8_t p) {
    const uint64_t q = uint64_t(v) >> p;

    write_elias_gamma(sink, q + 1);
    sink.write_int(v, p); // r is exactly the lowest p bits of v
}

template<typename T, typename BitSink>
inline T read_rice(BitSink&& sink, uint8_t p) {
    const auto q = read_elias_gamma<uint64_t>(sink) - 1;
    const auto r = sink.template read_int<uint64_t>(p);
    return T(q * (1ULL << p) + r);
}

}
}
