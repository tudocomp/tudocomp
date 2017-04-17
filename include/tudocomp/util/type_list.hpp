#pragma once

namespace tdc {

/// \brief Contains meta-programming utilities for type lists.
namespace tl {

/// \brief Meta-type for a list of types.
template<typename... Ts> struct type_list;

/// \brief An empty type list.
using mt = type_list<>;

/// \brief Represents "no" type.
struct None;

/// \brief Represents an unidentifiable, ambiguous type.
struct Ambiguous;

/// \cond INTERNAL

// decl
template<size_t I, typename Tl> struct _get;

// recursive case (I > 0): cut off head and continue with I - 1
template<size_t I, typename Head, typename... Tail>
struct _get<I, type_list<Head, Tail...>>
: _get<I - 1, type_list<Tail...>>{};

// trivial case (I = 0): yield head
template<typename Head, typename... Tail>
struct _get<0, type_list<Head, Tail...>> {
    using type = Head;
};
/// \endcond

/// \brief Gets the i-th type in a type list.
///
/// The result type can be obtained via the \c type member.
///
/// \tparam I  the index in the type list
/// \tparam Tl the type list in question
template<size_t I, typename Tl>
using get = typename _get<I, Tl>::type;

/// \cond INTERNAL

// decl
template<typename T, typename Tl> struct _prepend;

// impl
template<typename T, typename... Ts>
struct _prepend<T, type_list<Ts...>> {
    using list = type_list<T, Ts...>;
};

/// \endcond

/// \brief Prepends a type to a type list.
///
/// The resulting type list can be obtained via the \c list member.
///
/// \tparam T  the type to prepend
/// \tparam Tl the type list to modify
template<typename T, typename Tl>
using prepend = typename _prepend<T, Tl>::list;

/// \cond INTERNAL

// recursive case (I > 0):
// produce a type list of None and append set with I-1
template<size_t I, typename T> struct _set {
    using list = prepend<None, typename _set<I - 1, T>::list>;
};

// trivial case (I = 0): produce a type list of just T
template<typename T> struct _set<0, T> {
    using list = type_list<T>;
};

/// \endcond

/// \brief Produces a type list where the i-th type is the specified type
///        and all other types are None.
///
/// The resulting type list can be obtained via the \c list member.
///
/// \tparam I  the index
/// \tparam T  the type to set at the specified index
template<size_t I, typename T>
using set = typename _set<I, T>::list;

/// \cond INTERNAL

// decl
template<typename Tl1, typename Tl2> struct _mix;

// case 1: Head1 != None, Head2 != None => Ambiguous
template<typename Head1, typename... Tail1, typename Head2, typename... Tail2>
struct _mix<type_list<Head1, Tail1...>, type_list<Head2, Tail2...>> {
    using list = prepend<Ambiguous,
        typename _mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >;
};

// case 2: Head1 != None, Head2 == None => Head1
template<typename Head1, typename... Tail1, typename... Tail2>
struct _mix<type_list<Head1, Tail1...>, type_list<None, Tail2...>> {
    using list = prepend<Head1,
        typename _mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >;
};

// case 3: Head1 == None, Head2 != None => Head2
template<typename... Tail1, typename Head2, typename... Tail2>
struct _mix<type_list<None, Tail1...>, type_list<Head2, Tail2...>> {
    using list = prepend<Head2,
        typename _mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >;
};

// case 4: Head1 == None, Head2 == None => None
template<typename... Tail1, typename... Tail2>
struct _mix<type_list<None, Tail1...>, type_list<None, Tail2...>> {
    using list = prepend<None,
        typename _mix<
            type_list<Tail1...>,
            type_list<Tail2...>
        >::list
    >;
};

// trivial case: mix empty list with a non-empty list
template<typename Head1, typename... Tail1>
struct _mix<type_list<Head1, Tail1...>, mt> {
    using list = prepend<Head1,
        typename _mix<type_list<Tail1...>, mt>::list>;
};

template<typename Head2, typename... Tail2>
struct _mix<mt, type_list<Head2, Tail2...>> {
    using list = prepend<Head2,
        typename _mix<mt, type_list<Tail2...>>::list>;
};

// trivial case: mixing two empty lists yields an empty list
template<> struct _mix<mt, mt> {
    using list = mt;
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
/// \tparam Tl1 the fist type list
/// \tparam Tl2 the second type list
template<typename Tl1, typename Tl2>
using mix = typename _mix<Tl1, Tl2>::list;

/// \cond INTERNAL
// decl
template<typename... Tls> struct _multimix;

// recursive case: mix first two in list, then continue with that and remainder
template<typename Tl1, typename Tl2, typename... Tls>
struct _multimix<Tl1, Tl2, Tls...> {
    using list =
        typename _multimix<
            mix<Tl1, Tl2>,
            Tls...
        >::list;
};

// trivial case: multimixing one list yields the same list
template<typename Tl>
struct _multimix<Tl> {
    using list = Tl;
};
/// \endcond

/// \brief Mixes multiple lists.
///
/// Applies \ref mix to each consecutive pair of type lists.
/// The resulting type list can be obtained via the \c list member.
///
/// tparam Tls the lists to mix
template<typename... Tls>
using multimix = typename _multimix<Tls...>::list;

}} //ns
