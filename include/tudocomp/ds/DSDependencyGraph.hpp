#pragma once

#include <forward_list>
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
    struct Node {
        dsid_t      dsid;
        bool        requested;
        DSProvider* provider;

        inline Node(dsid_t _id, bool _req, DSProvider* _prov)
            : dsid(_id), requested(_req), provider(_prov) {
        }
    };

    using NodePtr = std::shared_ptr<Node>;
    using Edge = std::pair<NodePtr, NodePtr>;

    manager_t* m_manager;
    std::map<dsid_t, NodePtr> m_nodes;
    std::set<Edge>            m_edges;

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

    NodePtr create_node(dsid_t id, bool requested = false) {
        DLOG(INFO) << "node: " << ds::name_for(id);
        auto& provider = m_manager->get_provider(id);
        auto node = std::make_shared<Node>(id, requested, &provider);
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
            DLOG(INFO) << "edge: (" << ds::name_for(req_id) << ", " << ds::name_for(node->dsid) << ")";
            m_edges.emplace(req, node);
        }
    }

public:
    inline void insert_requested(dsid_t id) {
        auto node = find(id);
        if(node) {
            // ds already in the graph, mark as "requested"
            node->requested = true;
        } else {
            // insert new node
            insert(create_node(id, true));
        }
    }
};

} //ns

