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
class RollingTrie : public Algorithm, public LZ78Trie<factorid_t> {
    typedef typename HashRoller::key_type key_type;
    mutable HashRoller m_roller;
    HashMap<key_type, factorid_t, undef_id, HashFunction, std::equal_to<key_type>, HashProber, HashManager> m_table;

    inline key_type hash_node(uliteral_t c) const {
        m_roller += c;
        return m_roller();
    }

public:
    inline static Meta meta() {
        Meta m("lz78trie", "rolling", "Rolling Hash Trie");
        m.option("hash_roller").templated<HashRoller, ZBackupRollingHash>("hash_roller");
        m.option("hash_prober").templated<HashProber, LinearProber>("hash_prober");
        m.option("hash_manager").templated<HashManager, SizeManagerPrime>("hash_manager");
        m.option("load_factor").dynamic(30);
        m.option("hash_function").templated<HashFunction, NoopHasher>("hash_function");
        return m;
    }
    inline RollingTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(env))
        , LZ78Trie(n,remaining_characters)
        , m_roller(this->env().env_for_option("hash_roller"))
        , m_table(this->env(), n, remaining_characters) {
        m_table.max_load_factor(this->env().option("load_factor").as_integer()/100.0f );
        if(reserve > 0) {
            m_table.reserve(reserve);
        }
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~RollingTrie() {
            if (m_guard) {
                m_table.collect_stats(env());
            }
        }
    )
    RollingTrie(RollingTrie&& other) = default;
    RollingTrie& operator=(RollingTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        m_table.insert(std::make_pair<key_type,factorid_t>(hash_node(c), size()));
        m_roller.clear();
        return size() - 1;
    }

    inline node_t get_rootnode(uliteral_t c) const {
        hash_node(c);
        return c;
    }

    inline void clear() {
//        m_table.clear();
    }

    inline node_t find_or_insert(const node_t&, uliteral_t c) {
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        auto ret = m_table.insert(std::make_pair(hash_node(c), newleaf_id));
        if(ret.second) {
            m_roller.clear();
            return undef_id; // added a new node
        }
        return ret.first.value();
    }

    inline factorid_t size() const {
        return m_table.entries();
    }
};

}} //ns

