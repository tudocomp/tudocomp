#pragma once

#include <climits>
#include <unistd.h>

#include <glog/logging.h>

namespace tdc {

/*
    A `WriteBitSink` has to provide the following methods:

    ```
    void write_bit(bool set);

    template<typename T>
    void write_int(T value, size_t bits = sizeof(T) * CHAR_BIT);
    ```

    A `ReadBitSink` has to provide the following methods:

    ```
    uint8_t read_bit();

    template<class T>
    T read_int(size_t amount = sizeof(T) * CHAR_BIT);
    ```
*/

// TODO: Integrate or share with the tudocomp impls

template<class T, typename WriteBitSink>
inline void write_int(WriteBitSink&& sink, T value, size_t bits = sizeof(T) * CHAR_BIT) {
    for (int i = bits - 1; i >= 0; i--) {
        sink.write_bit((value & T(T(1) << i)) != T(0));
    }
}

template<class T, typename ReadBitSink>
inline T read_int(ReadBitSink&& sink, size_t amount = sizeof(T) * CHAR_BIT) {
    T value = 0;
    for(size_t i = 0; i < amount; i++) {
        value <<= 1;
        value |= sink.read_bit();
    }
    return value;
}

template<typename T, typename WriteBitSink>
inline void write_compressed_int(WriteBitSink&& sink, T v, size_t b = 7) {
    DCHECK_GT(b, 0U);

    uint64_t u = uint64_t(v);
    uint64_t mask = (u << b) - 1;
    do {
        uint64_t current = v & mask;
        v >>= b;

        sink.write_bit(v > 0);
        sink.write_int(current, b);
    } while(v > 0);
}

template<typename T, typename ReadBitSink>
inline T read_compressed_int(ReadBitSink&& sink, size_t b = 7) {
    DCHECK_GT(b, 0U);

    uint64_t value = 0;
    size_t i = 0;

    bool has_next;
    do {
        has_next = sink.read_bit();
        value |= (sink.template read_int<size_t>(b) << (b * (i++)));
    } while(has_next);

    return T(value);
}

template<typename T, typename WriteBitSink>
inline void write_unary(WriteBitSink&& sink, T v) {
    while(v--) {
        sink.write_bit(0);
    }

    sink.write_bit(1);
}

template<typename T, typename ReadBitSink>
inline T read_unary(ReadBitSink&& sink) {
    T v = 0;
    while(!sink.read_bit()) ++v;
    return v;
}

template<typename T, typename WriteBitSink>
inline void write_ternary(WriteBitSink&& sink, T v) {
    if(v) {
        --v;
        do {
            sink.write_int(v % 3, 2); // 0 -> 00, 1 -> 01, 2 -> 10
            v /= 3;
        } while(v);
    }
    sink.write_int(3, 2); // terminator -> 11
}

template<typename T, typename ReadBitSink>
inline T read_ternary(ReadBitSink&& sink) {
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

// Implementierung gemäß https://en.wikipedia.org/wiki/Elias_gamma_coding
template<typename T, typename WriteBitSink>
inline void write_elias_gamma(WriteBitSink&& sink, T v) {
    DCHECK_GT(v, T(0)) << "zero cannot be gamma-encoded";

    const auto m = bits_for(v) - 1;
    write_unary(sink, m);
    if(m > 0) sink.write_int(v, m); // cut off leading 1
}

// Implementierung gemäß https://en.wikipedia.org/wiki/Elias_gamma_coding
template<typename T, typename ReadBitSink>
inline T read_elias_gamma(ReadBitSink&& sink) {
    auto m = read_unary<size_t>(sink);
    if(m > 0) {
        return T((1ULL << m) | sink.template read_int<uint64_t>(m));
    } else {
        return T(1);
    }
}

template<typename T, typename WriteBitSink>
inline void write_elias_delta(WriteBitSink&& sink, T v) {
    DCHECK_GT(v, T(0)) << "zero cannot be delta-encoded";

    auto m = bits_for(v) - 1;
    write_elias_gamma(sink, m+1);
    if(m > 0) sink.write_int(v, m); // cut off leading 1
}

template<typename T, typename ReadBitSink>
inline T read_elias_delta(ReadBitSink&& sink) {
    auto m = read_elias_gamma<size_t>(sink) - 1;
    if(m > 0) {
        return T((1ULL << m) | sink.template read_int<uint64_t>(m));
    } else {
        return T(1);
    }
}

template<typename T, typename WriteBitSink>
inline void write_rice(WriteBitSink&& sink, T v, uint8_t p) {
    const uint64_t q = uint64_t(v) >> p;

    write_elias_gamma(sink, q + 1);
    sink.write_int(v, p); // r is exactly the lowest p bits of v
}

template<typename T, typename ReadBitSink>
inline T read_rice(ReadBitSink&& sink, uint8_t p) {
    const auto q = read_elias_gamma<uint64_t>(sink) - 1;
    const auto r = sink.template read_int<uint64_t>(p);
    return T(q * (1ULL << p) + r);
}

}
