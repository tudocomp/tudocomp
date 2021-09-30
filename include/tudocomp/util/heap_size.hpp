#pragma once

#include <iostream>
#include <typeinfo>
#include <tuple>
#include <memory>

#include <tudocomp/util/object_size_t.hpp>

namespace tdc {
    template<typename T>
    struct heap_size {
        static object_size_t compute(T const& val) {
            return object_size_t::unknown_extra_data(sizeof(T));
        }
    };

    template<typename T>
    inline object_size_t heap_size_compute(T const& val) {
        return heap_size<T>::compute(val);
    }

#define gen_heap_size_without_indirection(...) \
    template<>\
    struct heap_size<__VA_ARGS__> {\
        static object_size_t compute(__VA_ARGS__ const& val) {\
            return object_size_t::exact(sizeof(__VA_ARGS__));\
        }\
    };

    gen_heap_size_without_indirection(bool)
    gen_heap_size_without_indirection(unsigned char)
    gen_heap_size_without_indirection(signed char)
    gen_heap_size_without_indirection(char)
    gen_heap_size_without_indirection(unsigned short int)
    gen_heap_size_without_indirection(unsigned int)
    gen_heap_size_without_indirection(unsigned long int)
    gen_heap_size_without_indirection(unsigned long long int)
    gen_heap_size_without_indirection(signed short int)
    gen_heap_size_without_indirection(signed int)
    gen_heap_size_without_indirection(signed long int)
    gen_heap_size_without_indirection(signed long long int)
    gen_heap_size_without_indirection(float)
    gen_heap_size_without_indirection(double)

    template<typename T>
    struct heap_size<std::unique_ptr<T[]>> {
        static object_size_t compute(std::unique_ptr<T[]> const& val, size_t size) {
            auto bytes = object_size_t::exact(sizeof(val));

            for (size_t i = 0; i < size; i++) {
                bytes += heap_size<T>::compute(val[i]);
            }

            return bytes;
        }
    };
}
