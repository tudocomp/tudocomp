#pragma once

#include <tudocomp/util/cpp14/integer_sequence.hpp>

namespace tdc {
namespace is {

/// \cond INTERNAL

// declarations

template<typename T, T Value, typename Seq> struct _prepend;
template<typename T, typename Compare, T Value, typename Seq> struct _insert;
template<typename T, typename Compare, typename Seq> struct _sort;

/// \endcond

// prepend - decl

/// \brief Prepends a value to an integer sequence.
///
/// \tparam T the integer type
/// \tparam Value the value to prepend
/// \tparam Seq the sequence to prepend to
template<typename T, T Value, typename Seq>
using prepend = typename _prepend<T, Value, Seq>::seq;

/// \brief Ascending comparator for insertion sort.
///
/// The comparator returns \c true iff the first value is less than or equal
/// to the second using the \c <= operator.
struct ascending {
    template<typename T, T A, T B>
    static constexpr bool compare() {
        return A <= B;
    }
};

/// \brief Descending comparator for insertion sort.
///
/// The comparator returns \c true iff the first value is greater than or equal
/// to the second using the \c >= operator.
struct descending {
    template<typename T, T A, T B>
    static constexpr bool compare() {
        return A >= B;
    }
};

/// \brief Inserts an integer value into a sorted sequence while retaining the
///        ordering.
///
/// \tparam T the integer type
/// \tparam Value the value to insert
/// \tparam Seq the sorted integer sequence to insert into
/// \tparam Compare the value comparator. Must implement a special \c compare
///         function. See \ref ascending and \ref descending for examples.
template<typename T, T Value, typename Seq, typename Compare = ascending>
using insert = typename _insert<T, Compare, Value, Seq>::seq;

/// \brief Sorts an integer sequence using insertion sort.
///
/// \tparam T the integer type
/// \tparam Seq the sequence to sort
/// \tparam Compare the value comparator. Must implement a special \c compare
///         function. See \ref ascending and \ref descending for examples.
template<typename T, typename Seq, typename Compare = ascending>
using sort = typename _sort<T, Compare, Seq>::seq;

/// \brief Sorts an index sequence using insertion sort.
///
/// \tparam Seq the sequence to sort
/// \tparam Compare the value comparator. Must implement a
///         <code>static constexpr bool compare(T a, T b)</code> function that
///         returns \c true iff a comes before b in the ordering.
template<typename Seq, typename Compare = ascending>
using sort_idx = typename _sort<size_t, Compare, Seq>::seq;

/// \cond INTERNAL

// implementations

// prepend - impl
template<typename T, T Value, T... Seq>
struct _prepend<T, Value, std::integer_sequence<T, Seq...>> {
    using seq = std::integer_sequence<T, Value, Seq...>;
};

// insert operation - case 1: Value <= Head
// simply prepend Value to remaining sequence
template<typename T, typename Compare, T Value, T Head, T... Tail>
constexpr typename std::enable_if<Compare::template compare<T, Value, Head>(),
    std::integer_sequence<T, Value, Head, Tail...>
    >::type _insert_op();

// insert operation - case 2: Value > Head
// prepend Head to recursive application
template<typename T, typename Compare, T Value, T Head, T... Tail>
constexpr typename std::enable_if<!Compare::template compare<T, Value, Head>(),
    prepend<T, Head, insert<T, Value, std::integer_sequence<T, Tail...>, Compare>>
    >::type _insert_op();

// insert - trivial case (sequence is empty)
template<typename T, typename Compare, T Value>
struct _insert<T, Compare, Value, std::integer_sequence<T>> {
    using seq = std::integer_sequence<T, Value>;
};

// insert - recursive case
// prepend value iff Head is less or equal
template<typename T, typename Compare, T Value, T Head, T... Tail>
struct _insert<T, Compare, Value, std::integer_sequence<T, Head, Tail...>> {
    using seq = decltype(_insert_op<T, Compare, Value, Head, Tail...>());
};

// insertion sort - trivial case
// empty sequence
template<typename T, typename Compare>
struct _sort<T, Compare, std::integer_sequence<T>> {
    using seq = std::integer_sequence<T>;
};

// insertion sort - recursive case
// insert next value into sequence
template<typename T, typename Compare, T Head, T... Tail>
struct _sort<T, Compare, std::integer_sequence<T, Head, Tail...>> {
    using seq = insert<T, Head,
        sort<T, std::integer_sequence<T, Tail...>, Compare>,
        Compare>;
};

/// \endcond

/// \cond INTERNAL
template<typename T>
inline void _to_vector(std::vector<T>& v, std::integer_sequence<T>) {
    // done
}

template<typename T, T Head, T... Tail>
inline void _to_vector(std::vector<T>& v,
    std::integer_sequence<T, Head, Tail...>) {

    // add head and recurse
    v.emplace_back(Head);
    _to_vector(v, std::integer_sequence<T, Tail...>());
}
/// \endcond

/// \brief Creates a vector from an integer sequence
///
/// \tparam T the integer type
/// \tparam Seq the integer sequence
template<typename T, T... Seq>
inline std::vector<T> to_vector(std::integer_sequence<T, Seq...>) {
    std::vector<T> v;
    v.reserve(sizeof...(Seq));
    _to_vector(v, std::integer_sequence<T, Seq...>());
    return v;
}

}} //ns
