#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>

namespace tdc {

namespace lz78 {

template<
    typename HashRoller = ZBackupRollingHash,
    typename HashProber = LinearProber,
    typename HashManager = SizeManagerPrime,
    typename HashFunction = NoopHasher
>
class RollingTrie : public Algorithm, public LZ78Trie<> {
    typedef typename HashRoller::key_type key_type;
    mutable HashRoller m_roller;
    HashMap<key_type, factorid_t, undef_id, HashFunction, std::equal_to<key_type>, HashProber, HashManager> m_table;

    inline key_type hash_node(uliteral_t c) const {
        m_roller += c;
        return m_roller();
    }

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "rolling", "Rolling Hash Trie");
        m.param("hash_roller").strategy<HashRoller>(hash_roller_type(), Meta::Default<ZBackupRollingHash>());
        m.param("hash_prober").strategy<HashProber>(hash_prober_type(), Meta::Default<LinearProber>());
        m.param("hash_manager").strategy<HashManager>(hash_manager_type(), Meta::Default<SizeManagerPrime>());
        m.param("hash_function").strategy<HashFunction>(hash_function_type(), Meta::Default<NoopHasher>());
        m.param("load_factor").primitive(30);
        return m;
    }
    inline RollingTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n,remaining_characters)
        , m_roller(this->config().sub_config("hash_roller"))
        , m_table(this->config(), n, remaining_characters) {
        m_table.max_load_factor(this->config().param("load_factor").as_float()/100.0f );
        if(reserve > 0) {
            m_table.reserve(reserve);
        }
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~RollingTrie() {
            if (m_guard) {
                m_table.collect_stats(config());
            }
        }
    )
    RollingTrie(RollingTrie&& other) = default;
    RollingTrie& operator=(RollingTrie&& other) = default;

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

        auto ret = m_table.insert(std::make_pair(hash_node(c), newleaf_id));
        if(ret.second) {
            m_roller.clear();
            return node_t(newleaf_id, true); // added a new node
        }
        return node_t(ret.first.value(), false);
    }

    inline size_t size() const {
        return m_table.entries();
    }
};

}} //ns

