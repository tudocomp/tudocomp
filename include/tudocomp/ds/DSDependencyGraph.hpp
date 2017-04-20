#pragma once

#include <tudocomp/util/cpp14/integer_sequence.hpp>
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

    // in_degree - trivial case
    // return size of requirement list
    template<dsid_t ds> struct _in_degree {
        static constexpr size_t value = provider_t<ds>::requires::size();
    };

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
        static constexpr size_t value = _in_degree<ds>::value;
    };

public:
    template<dsid_t ds>
    static constexpr size_t in_degree() {
        return _in_degree<ds>::value;
    }

    template<dsid_t ds>
    static constexpr size_t cost() {
        return _cost<ds, typename provider_t<ds>::requires>::value;
    }
};

} //ns

