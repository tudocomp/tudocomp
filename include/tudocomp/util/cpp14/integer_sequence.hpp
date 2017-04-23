#pragma once

#include <cstddef>
#include <vector>

namespace std {

/*
    This header simulates the std::integer_sequence as introduced in C++14.
    Remove this file when tudocomp is adapted to C++14.

    Source: http://www.pdimov.com/cpp2/simple_cxx11_metaprogramming.html
*/

template<class T, T... Ints> struct integer_sequence
{
    static constexpr std::size_t size() {
        return sizeof...(Ints);
    }
};

template<class S> struct next_integer_sequence;

template<class T, T... Ints> struct next_integer_sequence<integer_sequence<T, Ints...>>
{
    using type = integer_sequence<T, Ints..., sizeof...(Ints)>;
};

template<class T, T I, T N> struct make_int_seq_impl;

template<class T, T N>
    using make_integer_sequence = typename make_int_seq_impl<T, 0, N>::type;

template<class T, T I, T N> struct make_int_seq_impl
{
    using type = typename next_integer_sequence<
        typename make_int_seq_impl<T, I+1, N>::type>::type;
};

template<class T, T N> struct make_int_seq_impl<T, N, N>
{
    using type = integer_sequence<T>;
};

template<std::size_t... Ints>
    using index_sequence = integer_sequence<std::size_t, Ints...>;

template<std::size_t N>
    using make_index_sequence = make_integer_sequence<std::size_t, N>;

} // namespace std
