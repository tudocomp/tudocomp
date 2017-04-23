#pragma once

#include <map>
#include <set>

#include <tudocomp/util/integer_sequence.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/CompressMode.hpp>

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
/// its required data structures. The degree of a node is the amount of paths
/// from that node to the CONSTRUCT node (see below), which is equal to its
/// out-degree.
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
/// \tparam manager_t the data structure manager's type
/// \tparam m_construct the data structures to construct
template<typename manager_t, dsid_t... m_construct>
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
    /// \brief Computes the construction order for a data structure node's
    ///        incoming edges based on their costs (highest first).
    ///
    /// \tparam ds the data structure
    template<dsid_t ds>
    using dependency_order = typename _construction_order<
        typename provider_t<ds>::requires>::seq;

    /// \brief Computes the construction order for a set of data structure
    ///        nodes based on their costs (highest first).
    ///
    /// \tparam ds the data structures in question
    template<dsid_t... ds>
    using construction_order = typename _construction_order<
        std::index_sequence<ds...>>::seq;

private:
    manager_t* m_manager;
    CompressMode m_cm;

    std::map<dsid_t, size_t> m_degree;
    std::set<dsid_t> m_constructed;

    inline void init_degree(std::index_sequence<>) {
        // end of a construction path
    }

    template<dsid_t Head, dsid_t... Tail>
    inline void init_degree(std::index_sequence<Head, Tail...>) {
        // init degree for dependencies (any order)
        init_degree(typename provider_t<Head>::requires());

        // increase degree
        auto it = m_degree.find(Head);
        if(it == m_degree.end()) {
            m_degree.emplace(Head, 1);
        } else {
            it->second += 1;
        }

        // next
        init_degree(std::index_sequence<Tail...>());
    }

    inline void decrease_degree(std::index_sequence<>) {
        // end
    }

    template<dsid_t Head, dsid_t... Tail>
    inline void decrease_degree(std::index_sequence<Head, Tail...>) {
        auto it = m_degree.find(Head);
        if(it == m_degree.end()) {
            throw std::logic_error(
                std::string("decrease degree for orphan node ") +
                ds::name_for(Head));
        } else if(!it->second) {
            throw std::logic_error(
                std::string("degree already zero for node ") +
                ds::name_for(Head));
        }

        // decrease degree
        if(!(--it->second)) {
            // no longer needed
            DLOG(INFO) << "discard: " << ds::name_for(Head);
        }

        // next
        decrease_degree(std::index_sequence<Tail...>());
    }

    inline void discard_byproducts(std::index_sequence<>) {
        // end
    }

    template<dsid_t Head, dsid_t... Tail>
    inline void discard_byproducts(std::index_sequence<Head, Tail...>) {
        if(m_degree.find(Head) == m_degree.end()) {
            // not in the dependency graph
            DLOG(INFO) << "discard byproduct: " << ds::name_for(Head);
        }

        // next
        discard_byproducts(std::index_sequence<Tail...>());
    }

    inline void construct_recursive(std::index_sequence<>) {
        // end of a construction path
    }

    template<dsid_t Head, dsid_t... Tail>
    inline void construct_recursive(std::index_sequence<Head, Tail...>) {
        if(m_constructed.find(Head) == m_constructed.end()) {
            // construct dependencies
            construct_recursive(dependency_order<Head>());

            // construct
            DLOG(INFO) << "construct: " << ds::name_for(Head);

            // mark as constructed
            m_constructed.emplace(Head);

            // discard byproducts
            discard_byproducts(typename provider_t<Head>::provides());

            // decrease degree of direct dependencies
            decrease_degree(typename provider_t<Head>::requires());

            // next
            construct_recursive(std::index_sequence<Tail...>());
        }
    }

public:
    /// \brief Constructs the requested data structures in memory peak
    ///        optimized order.
    ///
    /// \param manager the data structure manager instance
    /// \param cm the compression mode to use
    inline DSDependencyGraph(manager_t& manager, const CompressMode cm)
            : m_manager(&manager), m_cm(cm) {

        // init degree by walking each construction path once in advance
        init_degree(std::index_sequence<m_construct...>());

        // construct data structures
        construct_recursive(construction_order<m_construct...>());
    }
};

} //ns

