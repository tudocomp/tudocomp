#pragma once

#include <tudocomp/util/cpp14/integer_sequence.hpp>
#include <tudocomp/util/integer_sequence_sort.hpp>
#include <tudocomp/ds/DSDef.hpp>

namespace tdc {

/// \brief Implements a dependency graph for data structures and provides
///        functionality for memory peak efficient construction.
///
/// A data structure is "relevant" iff it has either been requested by the
/// client or if it is required for a requested data structure to be
/// constructed. Non-relevant data structures are called "byproducts".
///
/// The graph contains a node for each relevant data structure. In each node,
/// we store a pointer to the provider, a "cost" value and a "degree" value.
///
/// There is an edge from node A to node B iff B requires A for construction.
///
/// The cost of a node equals its in-degree added to the cumulated costs of
/// its required data structures. The out-degree of a node is simply called
/// "degree".
///
/// The construction process consists of two phases: request and evaluation.
///
/// In the request phase, requested data structures are inserted to the graph
/// along with their requirements (recursively). They are then connected to a
/// virtual terminal node entitled CONSTRUCT.
///
/// In the evaluation phase, starting with CONSTRUCT, each ingoing edge is
/// followed in cost order and the respective data structure is constructed
/// recursively.
/// After each of these steps, the degree of each node on the requirement path
/// is decreased by one. For any non-requested node whose degree reaches zero,
/// the corresponding data structure is discarded. Byproducts (produced data
/// structures that have no corresponding node in the graph) are discarded
/// immediately.
///
template<typename manager_t>
class DSDependencyGraph {
private:
    // shortcut to provider type for a certain data structure
    template<dsid_t ds>
    using provider_t = typename manager_t::template provider_type<ds>;

    // in_degree = size of requirement list
    template<dsid_t ds> struct _in_degree {
        static constexpr size_t value = provider_t<ds>::requires::size();
    };

public:
    /// \brief Returns the in-degree of a data structure node.
    ///
    /// \tparam ds the data structure
    template<dsid_t ds>
    static constexpr size_t in_degree() {
        return _in_degree<ds>::value;
    }

private:
    // cost - decl
    template<dsid_t ds, typename req> struct _cost;

    // cost - recursive case (more requirements left to process)
    // add cost (recursive) of next ingoing node to cost of remainder
    template<dsid_t ds, dsid_t req_head, dsid_t... req_tail>
    struct _cost<ds, std::index_sequence<req_head, req_tail...>> {
        static constexpr size_t value =
            _cost<req_head, typename provider_t<req_head>::requires>::value +
            _cost<ds, std::index_sequence<req_tail...>>::value;
    };

    // cost - trivial case (no more requirements)
    // return the in-degree of the node
    template<dsid_t ds>
    struct _cost<ds, std::index_sequence<>> {
        static constexpr size_t value = in_degree<ds>();
    };

public:
    /// \brief Returns the cost of a data structure node.
    ///
    /// \tparam ds the data structure
    template<dsid_t ds>
    static constexpr size_t cost() {
        return _cost<ds, typename provider_t<ds>::requires>::value;
    }

private:
    // descending comparator for costs
    struct _cost_compare {
        template<typename T, T A, T B>
        static constexpr bool compare() {
            static_assert(std::is_same<T, dsid_t>::value, "bad dsid type");
            return cost<A>() >= cost<B>();
        }
    };

    // computes the construction order for a set of data structures based
    // on their costs (highest first)
    template<typename Seq>
    struct _construction_order {
        using seq = is::sort_idx<Seq, _cost_compare>;
    };

public:
    /// \brief Computes the construction order for a set of data structure
    ///        nodes based on their costs (highest first).
    ///
    /// \tparam ds the data structures in question
    template<dsid_t... ds>
    using construction_order = typename _construction_order<
        std::index_sequence<ds...>>::seq;

    /// \brief Computes the construction order for a data structure node's
    ///        incoming edges based on their costs (highest first).
    ///
    /// \tparam ds the data structure
    template<dsid_t ds>
    using dependency_order = typename _construction_order<
        typename provider_t<ds>::requires>::seq;
};

} //ns

