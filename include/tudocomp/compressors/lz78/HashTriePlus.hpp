#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>

namespace tdc {
namespace lz78 {


template<class HashFunction = MixHasher, class HashManager = SizeManagerDirect>
class HashTriePlus : public Algorithm, public LZ78Trie<> {
    HashMap<squeeze_node_t,factorid_t,undef_id,HashFunction,std::equal_to<squeeze_node_t>,LinearProber,SizeManagerPow2> m_table;
    HashMap<squeeze_node_t,factorid_t,undef_id,HashFunction,std::equal_to<squeeze_node_t>,LinearProber,HashManager> m_table2;

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "hash_plus", "Hash Trie+");
        m.param("hash_function").strategy<HashFunction>(hash_function_type(), Meta::Default<MixHasher>());
        m.param("hash_manager").strategy<HashManager>(hash_manager_type(), Meta::Default<SizeManagerDirect>());
        m.param("load_factor").primitive(30);
        return m;
    }

    inline HashTriePlus(Config&& cfg, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n,remaining_characters)
        , m_table(this->config(),n,remaining_characters)
        , m_table2(this->config(),n,remaining_characters)
    {
        m_table.max_load_factor(this->config().param("load_factor").as_float()/100.0f );
        m_table2.max_load_factor(0.95);
        if(reserve > 0) {
            m_table.reserve(reserve);
        }
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~HashTriePlus() {
            if (m_guard) {
                if(m_table2.empty()) {
                    m_table.collect_stats(config());
                } else {
                    m_table2.collect_stats(config());
                }
            }
        }
    )
    HashTriePlus(HashTriePlus&& other) = default;
    HashTriePlus& operator=(HashTriePlus&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        DCHECK(m_table2.empty());
        m_table.insert(std::make_pair<squeeze_node_t,factorid_t>(create_node(0, c), size()));
        return node_t(size() - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
//        table.clear();

    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary
        if(!m_table2.empty()) { // already using the second hash table
            auto ret = m_table2.insert(std::make_pair(create_node(parent+1,c), newleaf_id));
            if(ret.second) {
                return node_t(newleaf_id, true); // added a new node
            }
            return node_t(ret.first.value(), false);
        }
        // using still the first hash table
        auto ret = m_table.insert(std::make_pair(create_node(parent+1,c), newleaf_id));
        if(ret.second) {
            if(tdc_unlikely(m_table.table_size()*m_table.max_load_factor() < m_table.m_entries+1)) {
                const size_t expected_size = (m_table.m_entries + 1 + lz78_expected_number_of_remaining_elements(m_table.entries(),m_table.m_n,m_table.m_remaining_characters))/0.95;
                if(expected_size < m_table.table_size()*2.0*0.95) {
                    m_table2.incorporate(m_table, expected_size);
                }

            }
            return node_t(newleaf_id, true); // added a new node
        }
        return node_t(ret.first.value(), false);
    }

    inline size_t size() const {
        return m_table2.empty() ? m_table.entries() : m_table2.entries();
    }
};

}} //ns

