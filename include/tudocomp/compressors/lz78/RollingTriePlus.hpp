#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp/util/hash/rabinkarphash.h>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>

namespace tdc {

namespace lz78 {

template<
    typename HashRoller = KarpRabinHash64,
    typename HashFunction = NoopHasher,
    typename HashManager = SizeManagerNoop
>
class RollingTriePlus : public Algorithm, public LZ78Trie<> {
    typedef typename HashRoller::key_type key_type;
    mutable HashRoller m_roller;
    HashMap<key_type, factorid_t, undef_id, NoopHasher, std::equal_to<key_type>, LinearProber, SizeManagerPow2> m_table;
    HashMap<key_type, factorid_t, undef_id, HashFunction, std::equal_to<key_type>, LinearProber, HashManager> m_table2;

    inline key_type hash_node(uliteral_t c) const {
        m_roller += c;
        return m_roller();
    }

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "rolling_plus", "Rolling Hash Trie+");
        m.param("hash_roller").strategy<HashRoller>(hash_roller_type(), Meta::Default<KarpRabinHash64>());
        m.param("hash_function").strategy<HashFunction>(hash_function_type(), Meta::Default<NoopHasher>()); // dummy parameter
        m.param("load_factor").primitive(30);
        return m;
    }
    inline RollingTriePlus(Config&& cfg, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n,remaining_characters)
        , m_roller(this->config().sub_config("hash_roller"))
        , m_table(this->config(), n, remaining_characters)
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
        inline ~RollingTriePlus() {
            if (m_guard) {
                if(m_table2.empty()) {
                    m_table.collect_stats(config());
                } else {
                    m_table2.collect_stats(config());
                }
            }
        }
    )
    RollingTriePlus(RollingTriePlus&& other) = default;
    RollingTriePlus& operator=(RollingTriePlus&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        m_table.insert(std::make_pair<key_type,factorid_t>(hash_node(c), size()));
        m_roller.clear();
        return node_t(size() - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        hash_node(c);
        return node_t(c, false);
    }

    inline void clear() {
//        m_table.clear();
    }

    inline node_t find_or_insert(const node_t&, uliteral_t c) {
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary



        if(!m_table2.empty()) { // already using the second hash table
            auto ret = m_table2.insert(std::make_pair(hash_node(c), newleaf_id));
            if(ret.second) {
                m_roller.clear();
                return node_t(newleaf_id, true); // added a new node
            }
            return node_t(ret.first.value(), false);

        }
        // using still the first hash table
        auto ret = m_table.insert(std::make_pair(hash_node(c), newleaf_id));
        if(ret.second) {
            if(tdc_unlikely(m_table.table_size()*m_table.max_load_factor() < m_table.m_entries+1)) {
                const size_t expected_size = (m_table.m_entries + 1 + lz78_expected_number_of_remaining_elements(m_table.entries(),m_table.m_n,m_table.m_remaining_characters))/0.95;
                if(expected_size < m_table.table_size()*2.0*0.95) {
                    m_table2.incorporate(m_table, expected_size);
                }

            }
            m_roller.clear();
            return node_t(newleaf_id, true); // added a new node
        }
        return node_t(ret.first.value(), false);


    }

    inline factorid_t size() const {
        return m_table2.empty() ? m_table.entries() : m_table2.entries();
    }
};

}} //ns

