#pragma once

namespace tdc {

/// \brief Contains meta-programming utilities for type lists.
namespace type_list {

/// \brief Meta-type for a list of types.
template<typename... Ts> struct type_list;

/// \brief An empty type list.
using mt = type_list<>;

/// \brief Represents "no" type.
struct None;

/// \brief Represents an unidentifiable, ambiguous type.
struct Ambiguous;

/// \brief Gets i-th type in a type list.
///
/// The result type can be obtained via the \c type member.
///
/// \tparam I  the index in the type list
/// \tparam Tl the type list in question
template<size_t I, typename Tl> struct get;

/// \cond INTERNAL

// recursive case (I > 0): cut off head and continue with I - 1
template<size_t I, typename Head, typename... Tail>
struct get<I, type_list<Head, Tail...>>
: get<I - 1, type_list<Tail...>>{};

// trivial case (I = 0): yield head
template<typename Head, typename... Tail>
struct get<0, type_list<Head, Tail...>> {
    using type = Head;
};
/// \endcond

/// \brief Prepends a type to a type list.
///
/// The resulting type list can be obtained via the \c list member.
///
/// \tparam T  the type to prepend
/// \tparam Tl the type list to modify
template<typename T, typename Tl> struct prepend;

/// \cond INTERNAL

// impl
template<typename T, typename... Ts>
struct prepend<T, type_list<Ts...>> {
    using list = type_list<T, Ts...>;
};

/// \endcond

/// \brief Mixes two type lists.
///
/// The type lists must be equally long.
///
/// For each pair of types \c T1 and \c T2 in the two lists, the result list
/// will be determined using the following rules:
///
/// * \ref None if both \c T1 and \c T2 are \ref None.
/// * \c T1 if \c T1 is not \ref None and \c T2 is \ref None.
/// * \c T2 if \c T1 is \ref None and \c T2 is not \ref None.
/// * \ref Ambiguous if neither \c T1 or \c T2 are \ref None.
///
/// The resulting type list can be obtained via the \c list member.
///
/// \tparam Tl1 the fist type list
/// \tparam Tl2 the second type list
template<typename Tl1, typename Tl2> struct mix;

/// \cond INTERNAL

// case 1: Head1 != None, Head2 != None => Ambiguous
template<typename Head1, typename... Tail1, typename Head2, typename... Tail2>
struct mix<type_list<Head1, Tail1...>, type_list<Head2, Tail2...>> {
    using list = typename prepend<Ambiguous,
        typename mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >::list;
};

// case 2: Head1 != None, Head2 == None => Head1
template<typename Head1, typename... Tail1, typename... Tail2>
struct mix<type_list<Head1, Tail1...>, type_list<None, Tail2...>> {
    using list = typename prepend<Head1,
        typename mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >::list;
};

// case 3: Head1 == None, Head2 != None => Head2
template<typename... Tail1, typename Head2, typename... Tail2>
struct mix<type_list<None, Tail1...>, type_list<Head2, Tail2...>> {
    using list = typename prepend<Head2,
        typename mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >::list;
};

// case 4: Head1 == None, Head2 == None => None
template<typename... Tail1, typename... Tail2>
struct mix<type_list<None, Tail1...>, type_list<None, Tail2...>> {
    using list = typename prepend<None,
        typename mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >::list;
};

// trivial case: mix empty list with a non-empty list
template<typename Head1, typename... Tail1>
struct mix<type_list<Head1, Tail1...>, mt> {
    using list = typename prepend<Head1,
        typename mix<type_list<Tail1...>, mt>::list>::list;
};

template<typename Head2, typename... Tail2>
struct mix<mt, type_list<Head2, Tail2...>> {
    using list = typename prepend<Head2,
        typename mix<mt, type_list<Tail2...>>::list>::list;
};

// trivial case: mixing two empty lists yields an empty list
template<> struct mix<mt, mt> {
    using list = mt;
};
/// \endcond

/// \brief Mixes multiple lists.
///
/// Applies \ref mix to each consecutive pair of type lists.
/// The resulting type list can be obtained via the \c list member.
///
/// tparam Tls the lists to mix
template<typename... Tls> struct multimix;

/// \cond INTERNAL

// recursive case: mix first two in list, then continue with that and remainder
template<typename Tl1, typename Tl2, typename... Tls>
struct multimix<Tl1, Tl2, Tls...> {
    using list =
        typename multimix<
            typename mix<Tl1, Tl2>::list,
            Tls...
        >::list;
};

// trivial case: multimixing one list yields the same list
template<typename Tl>
struct multimix<Tl> {
    using list = Tl;
};
/// \endcond

}} //ns
