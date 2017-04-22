#pragma once

#include <cstddef>
#include <vector>

namespace std {

/*
    This header simulates the std::integer_sequence as introduced in C++14.
    Remove this std namespace part when tudocomp is adapted to C++14.

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

namespace tdc {

/// Gives the ith element of a given integer sequence.
template<size_t I, typename Seq> struct sequence_element;

/// Recursive case for sequence_element: strip off the first element in
/// the sequence and retrieve the (i-1)th element of the remaining sequence.
template<size_t I, typename T, T Head, T... Tail>
struct sequence_element<I, std::integer_sequence<T, Head, Tail...>>
: sequence_element<I - 1, std::integer_sequence<T, Tail...>>{};

/// Basis case for sequence_element: The first element is the one we're seeking.
template<typename T, T Head, T... Tail>
struct sequence_element<0, std::integer_sequence<T, Head, Tail...>> {
    static constexpr T value = Head;
};

/// \cond INTERNAL
template<typename T>
inline void _vec_from_seq(std::vector<T>& v, std::integer_sequence<T>) {
    // done
}

template<typename T, T Head, T... Tail>
inline void _vec_from_seq(std::vector<T>& v,
    std::integer_sequence<T, Head, Tail...>) {

    // set and recurse
    v.emplace_back(Head);
    _vec_from_seq(v, std::integer_sequence<T, Tail...>());
}
/// \endcond

/// create a vector from an integer sequence
template<typename T, T... Seq>
inline std::vector<T> vector_from_sequence(
    std::integer_sequence<T, Seq...>) {

    std::vector<T> v;
    v.reserve(sizeof...(Seq));
    _vec_from_seq(v, std::integer_sequence<T, Seq...>());
    return v;
}

} // namespace tdc
