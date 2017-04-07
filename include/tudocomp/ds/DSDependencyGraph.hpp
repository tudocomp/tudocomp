#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

/// \brief Implements a dependency graph for data structures and provides
///        functionality for memory peak efficient construction.
///
/// A data structure is "relevant" iff it has either been requested by the
/// client or if it is required for a requested data structure to be
/// constructed. Non-relevant data structures are called "side products".
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
/// followed in cost order and the respective data structure is constructed.
/// After this, the degree of each node on the requirement path is decreased by
/// one. For any non-requested node whose degree reaches zero, the corresponding
/// data structure can be discarded. Side products (produced data structured
/// that have no corresponding node in the graph) are discarded immediately.
///
template<typename manager_t>
class DSDependencyGraph {
private:
    struct Node; //fwd
    using NodePtr = std::shared_ptr<Node>;

    struct ConstructionOrder {
        inline bool operator()(const NodePtr& a, const NodePtr& b) const {
            // highest cost means construct first
            return a->cost > b->cost;
        }
    };

    struct Node {
        dsid_t      dsid;
        DSProvider* provider;

        bool requested;

        // cost = in degree + sum of costs of all dependencies
        // used to determine processing order
        size_t cost;
        std::set<NodePtr, ConstructionOrder> dependencies;

        // out degree
        // used to determine when the data structure can be discarded
        size_t degree;

        inline Node(dsid_t _id, DSProvider* _prov)
            : dsid(_id),
              provider(_prov),
              requested(false),
              cost(0),
              degree(0)
        {
        }
    };

    manager_t* m_manager;
    std::map<dsid_t, NodePtr> m_nodes;
    std::set<NodePtr, ConstructionOrder> m_construct;

public:
    inline DSDependencyGraph(manager_t& manager) : m_manager(&manager) {
    }

private:
    inline NodePtr find(dsid_t id) {
        auto it = m_nodes.find(id);
        if(it != m_nodes.end()) {
            return it->second;
        } else {
            return NodePtr(); // "null"
        }
    }

    NodePtr create_node(dsid_t id) {
        DLOG(INFO) << "node: (" << ds::name_for(id) << ")";
        auto& provider = m_manager->get_provider(id);
        auto node = std::make_shared<Node>(id, &provider);
        m_nodes[id] = node;
        return node;
    }

    inline void insert(NodePtr node) {
        // get required data structures
        for(auto req_id : node->provider->requirements()) {
            auto req = find(req_id);
            if(!req) {
                // create and insert recursively
                req = create_node(req_id);
                insert(req);
            }

            // create edge (req, node)
            DLOG(INFO) << "edge: (" << ds::name_for(req_id) << ") -> (" << ds::name_for(node->dsid) << ")";
            req->degree++;
            node->dependencies.insert(req);

            // update cost (in degree + sum cost of all dependency costs)
            node->cost += 1 + req->cost;
        }
    }

    inline void decrease(NodePtr node) {
        if(--node->degree == 0 && !node->requested) {
            // if node was not requested, discard data structure
            DLOG(INFO) << "discard (" << ds::name_for(node->dsid) << ")";
        }

        for(auto dep : node->dependencies) {
            decrease(dep);
        }
    }

    inline void construct(NodePtr node) {
        // construct dependencies first (in cost order)
        for(auto dep : node->dependencies) {
            construct(dep);
        }

        // construct
        DLOG(INFO) << "construct (" << ds::name_for(node->dsid) << "), cost = " << node->cost;
    }

public:
    inline void request(dsid_t id) {
        NodePtr node = find(id);
        if(!node) {
            // insert new node
            node = create_node(id);
            insert(node);
        }

        // connect to "construct" node
        DLOG(INFO) << "edge: (" << ds::name_for(id) << ") -> (CONSTRUCT)";
        m_construct.insert(node);
        node->requested = true;
    }

    inline void construct_requested() {
        DLOG(INFO) << "construct_requested";
        for(auto node : m_construct) {
            // recursively construct
            construct(node);

            // recursively decrease degree
            decrease(node);

            // discard side products
            for(auto prod : node->provider->products()) {
                if(!find(prod)) {
                    DLOG(INFO) << "discard side product (" << ds::name_for(prod) << ")";
                }
            }
        }

        // clean up
        m_construct.clear();
        m_nodes.clear();
    }
};

} //ns

