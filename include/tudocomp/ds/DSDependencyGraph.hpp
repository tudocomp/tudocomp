#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>

#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSProvider.hpp>

namespace tdc {

template<typename manager_t>
class DSDependencyGraph {
private:
    struct Node; //fwd
    using NodePtr = std::shared_ptr<Node>;

    struct Node {
        dsid_t      dsid;
        DSProvider* provider;

        size_t      cost;
        std::set<NodePtr> dependencies;

        inline Node(dsid_t _id, DSProvider* _prov)
            : dsid(_id), provider(_prov), cost(0) {
        }
    };

    struct ConstructionOrder {
        inline bool operator()(const NodePtr& a, const NodePtr& b) const {
            // highest cost means construct first
            return a->cost > b->cost;
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
            node->dependencies.insert(req);

            // update cost (in degree + sum cost of all dependency costs)
            node->cost += 1 + req->cost;
        }
    }

public:
    inline void insert_requested(dsid_t id) {
        NodePtr node = find(id);
        if(!node) {
            // insert new node
            node = create_node(id);
            insert(node);
        }

        // connect to "construct" node
        DLOG(INFO) << "edge: (" << ds::name_for(id) << ") -> (CONSTRUCT)";
        m_construct.insert(node);
    }

    inline void construct() {
        DLOG(INFO) << "construct";
        for(auto node : m_construct) {
            DLOG(INFO) << "cost(" << ds::name_for(node->dsid) << ") = " << node->cost;
        }
    }
};

} //ns

