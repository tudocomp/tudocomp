#pragma once

#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>

#include "cedar.hpp"

namespace tdc {
namespace lz78 {

namespace cedar {
    struct CedarSearchPos {
        size_t from;
    };
}

template<>
class TrieNode<cedar::CedarSearchPos> {
    using search_pos_t = cedar::CedarSearchPos;

    factorid_t m_id;
    bool m_is_new;
    search_pos_t m_search_pos;
public:
    TrieNode(factorid_t id, bool is_new, search_pos_t const& search_pos):
        m_id(id),
        m_is_new(is_new),
        m_search_pos(search_pos) {
            //DCHECK(id != NO_VALUE && id != NO_PATH);
        }
    TrieNode(): TrieNode(0, false, search_pos_t { 0 }) {}

    inline bool is_new() const { return m_is_new; }
    inline factorid_t id() const { return m_id; }
    inline search_pos_t const& search_pos() const { return m_search_pos; }
};

class CedarTrie: public Algorithm, public LZ78Trie<cedar::CedarSearchPos> {
    using cedar_factorid_t = lz78::factorid_t;
    // NB: this refers to different cedar namespace than defined in this file
    using cedar_t = ::cedar::da<cedar_factorid_t>;

    static constexpr uint8_t NULL_ESCAPE_ESCAPE_BYTE = 255;
    static constexpr uint8_t NULL_ESCAPE_REPLACEMENT_BYTE = 254;

    static constexpr cedar_factorid_t NO_VALUE = static_cast<cedar_factorid_t>(
        cedar_t::error_code::CEDAR_NO_VALUE);
    static constexpr cedar_factorid_t NO_PATH = static_cast<cedar_factorid_t>(
        cedar_t::error_code::CEDAR_NO_PATH);
    static constexpr cedar_factorid_t HIDDEN_ESCAPE_ID = -3; // NOTE: May not be -1 or -2

    class LzwRootSearchPosMap {
        std::array<cedar::CedarSearchPos, 256> m_array;
    public:
        inline cedar::CedarSearchPos get(uliteral_t c) const {
            DCHECK(c < m_array.size());
            return m_array[c];
        }
        inline void set(uliteral_t c, cedar::CedarSearchPos v) {
            DCHECK(c < m_array.size());
            m_array[c] = v;
        }
    };

    // unique_ptr only needed for reassignment
    std::unique_ptr<cedar_t> m_trie;
    cedar_factorid_t m_ids = 0;
    LzwRootSearchPosMap m_roots;

    inline node_t _find_or_insert(const node_t& parent, uliteral_t c, bool incr_id) {
        auto search_pos = parent.search_pos();

        auto letter = (const char*) &c;
        auto& from = search_pos.from;
        cedar_factorid_t searchResult;

        {
            size_t pos = 0;
            searchResult = m_trie->traverse(letter, from, pos, 1);
        }

        if(searchResult != NO_VALUE && searchResult != NO_PATH) {
            return node_t {
                searchResult - 1,
                false,
                search_pos,
            };
        } else {
            {
                size_t pos = 0;
                if (incr_id) {
                    m_trie->update(letter, from, pos, 1, ++m_ids);
                } else {
                    m_trie->update(letter, from, pos, 1, HIDDEN_ESCAPE_ID);
                }
            }
            return node_t {
                factorid_t(size() - 1ull),
                true,
                search_pos,
            };
        }
    }

    inline void _print(size_t from, size_t ind) {
        cedar_t& t = *m_trie;

        bool prev_empty = false;
        size_t prev_empty_min = 1;
        size_t prev_empty_max = 1;

        auto print_prev_empty = [&]() {
            if (prev_empty) {
                DLOG(INFO)
                    << std::setfill(' ')
                    << std::setw(ind)
                    << ""
                    << "["
                    << std::setfill('0')
                    << std::setw(3)
                    << prev_empty_min
                    << "]: "
                    << "-"
                    << "\n";
                if (prev_empty_min != prev_empty_max) {
                    DLOG(INFO)
                        << std::setfill(' ')
                        << std::setw(ind)
                        << ""
                        << "[...]\n";
                    DLOG(INFO)
                        << std::setfill(' ')
                        << std::setw(ind)
                        << ""
                        << "["
                        << std::setfill('0')
                        << std::setw(3)
                        << prev_empty_max
                        << "]: "
                        << "-"
                        << "\n";
                }
                prev_empty = false;
            }
        };

        for (size_t i = 1; i < 256; i++) {
            auto child_from = from;
            const char c = uint8_t(i);
            size_t pos = 0;
            auto r = t.traverse(&c, child_from, pos, 1);
            if (r != NO_PATH && r != NO_VALUE) {
                print_prev_empty();
                prev_empty_min = i + 1;
                DLOG(INFO)
                    << std::setfill(' ')
                    << std::setw(ind)
                    << ""
                    << "["
                    << std::setfill('0')
                    << std::setw(3)
                    << i
                    << std::setfill(' ')
                    << "]: "
                    << r
                    << "\n";
                _print(child_from, ind + 4);
            } else {
                prev_empty = true;
                prev_empty_max = i;
            }
        }
        print_prev_empty();
    }

    inline void print() {
        DLOG(INFO) << "\n";
        _print(0, 0);
        DLOG(INFO) << "\n";
    }

public:
    inline static Meta meta() {
        Meta m("lz78trie", "cedar", "Lempel-Ziv 78 Cedar Trie");
        return m;
    }

    CedarTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t = 0)
        : Algorithm(std::move(env))
		, LZ78Trie(n, remaining_characters)
        , m_trie(std::make_unique<cedar_t>()) {}

    inline node_t add_rootnode(const uliteral_t c) {
        cedar_factorid_t ids = c;
        DCHECK(m_ids == ids);
        m_ids++;

        cedar::CedarSearchPos search_pos;

        if (c != 0 && c != NULL_ESCAPE_ESCAPE_BYTE) {
            const char* letter = (const char*) &c;
            size_t from = 0;
            size_t pos = 0;
            m_trie->update(letter, from, pos, 1, ids);
            DCHECK(pos == 1);
            search_pos = cedar::CedarSearchPos{ from };
        } else {
            const char* letter;
            size_t from = 0;
            size_t pos;

            pos = 0;

            auto null_escape_byte = NULL_ESCAPE_ESCAPE_BYTE;
            letter = (const char*) &null_escape_byte;
            auto res = m_trie->traverse(letter, from, pos, 1);

            if (res == NO_PATH || res == NO_VALUE) {
                DCHECK(pos == 0);
                m_trie->update(letter, from, pos, 1, cedar_factorid_t(HIDDEN_ESCAPE_ID));
                DCHECK(pos == 1);
            }

            pos = 0;
            char c2 = c;
            if (c == 0) c2 = NULL_ESCAPE_REPLACEMENT_BYTE;
            letter = (const char*) &c2;
            m_trie->update(letter, from, pos, 1, ids);
            DCHECK(pos == 1);

            search_pos = cedar::CedarSearchPos{ from };
        }
        auto r = node_t(ids, true, search_pos);
        m_roots.set(c, search_pos);
        /*
        DLOG(INFO) << "add rootnode "
            << "char: " << int(c)
            << ", factor id: "
            << r.id() << ", from: "
            << r.search_pos().from;
        print();
        */
        return r;
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false, m_roots.get(c));
    }

    inline void clear() {
        // TODO: cedar seems to have a clear() method, but also
        // seems to have bugs in its implementation
        m_trie = std::make_unique<cedar_t>();
        m_ids = 0;
        m_roots = LzwRootSearchPosMap();
    }

    inline node_t find_or_insert(const node_t& parent, uliteral_t c) {
        node_t r;
        /*
        DLOG(INFO) << "find or insert "
            << "char: " << int(c)
            << ", factor id: "
            << parent.id() << ", from: "
            << parent.search_pos().from;
        */
        if (c == 0) {
            auto r1 = _find_or_insert(parent, NULL_ESCAPE_ESCAPE_BYTE, false);
            auto r2 = _find_or_insert(r1, NULL_ESCAPE_REPLACEMENT_BYTE, true);
            r = r2;
        } else if (c == NULL_ESCAPE_ESCAPE_BYTE) {
            auto r1 = _find_or_insert(parent, NULL_ESCAPE_ESCAPE_BYTE, false);
            auto r2 = _find_or_insert(r1, NULL_ESCAPE_ESCAPE_BYTE, true);
            r = r2;
        } else {
            r = _find_or_insert(parent, c, true);
        }
        //print();
        return r;
    }

    inline size_t size() const {
        return m_ids;
    }
};

}} //ns

