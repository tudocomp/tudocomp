#pragma once

#include <iostream>
#include <typeinfo>
#include <tuple>

#include <tudocomp/util/object_size_t.hpp>

namespace tdc {
    inline bool equal_diagnostic(bool v, char const* msg) {
        if (!v) {
            std::cerr << "not equal: " << msg << "\n";
        }
        return v;
    }
#define gen_equal_diagnostic(e) \
    equal_diagnostic(e, #e)

#define gen_equal_check(field, ...)                                          \
    gen_equal_diagnostic(                                                         \
        serialize<decltype(lhs.field)>::equal_check(lhs.field, rhs.field, ##__VA_ARGS__))

    template<typename T>
    struct serialize {
        //*
        static object_size_t write(std::ostream& out, T const& val) {
            CHECK(false) << "Need to specialize `tdc::serialize` for type " << typeid(T).name();
            return object_size_t::unknown_extra_data(0);
        }
        static T read(std::istream& in) {
            CHECK(false) << "Need to specialize `tdc::serialize` for type " << typeid(T).name();
        }
        static bool equal_check(T const& lhs, T const& rhs) {
            CHECK(false) << "Need to specialize `tdc::serialize` for type " << typeid(T).name();
            return false;
        }
        //*/
    };

    template<typename T>
    inline object_size_t serialize_write(std::ostream& out, T const& val) {
        return serialize<T>::write(out, val);
    }

    template<typename T>
    inline T serialize_read(std::istream& inp) {
        return serialize<T>::read(inp);
    }

    template<typename T>
    inline void serialize_read_into(std::istream& inp, T& out) {
        out = serialize<T>::read(inp);
    }

#define gen_direct_serialization(...) \
    template<>\
    struct serialize<__VA_ARGS__> {\
        using T = __VA_ARGS__;\
        static object_size_t write(std::ostream& out, T const& val) {\
            out.write((char const*) &val, sizeof(T));\
            return object_size_t::exact(sizeof(T));\
        }\
        static T read(std::istream& in) {\
            T val;\
            in.read((char*) &val, sizeof(T));\
            return val;\
        }\
        static bool equal_check(T const& lhs, T const& rhs) {\
            return gen_equal_diagnostic(lhs == rhs);\
        }\
    };

    gen_direct_serialization(bool)
    gen_direct_serialization(unsigned char)
    gen_direct_serialization(signed char)
    gen_direct_serialization(char)
    gen_direct_serialization(unsigned short int)
    gen_direct_serialization(unsigned int)
    gen_direct_serialization(unsigned long int)
    gen_direct_serialization(unsigned long long int)
    gen_direct_serialization(signed short int)
    gen_direct_serialization(signed int)
    gen_direct_serialization(signed long int)
    gen_direct_serialization(signed long long int)
    gen_direct_serialization(float)
    gen_direct_serialization(double)

}
