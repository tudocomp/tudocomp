#include "test/util.hpp"
#include <gtest/gtest.h>

#include <tudocomp/util.hpp>
#include <tudocomp/CreateAlgorithm.hpp>

#include <tudocomp/compressors/lz78/LZ78Trie.hpp>

using namespace tdc;
using namespace lz78;

struct TestTrieElement {
    uint8_t chr;
    uint64_t id;
    std::vector<TestTrieElement> children;

    TestTrieElement(uint8_t c, uint64_t i): chr(c), id(i) {}
    TestTrieElement(uint8_t c, uint64_t i, std::vector<TestTrieElement>&& v):
        chr(c),
        id(i),
        children(std::move(v)) {}

    TestTrieElement& find(uint8_t c) {
        for (auto& e : children) {
            if (e.chr == c) {
                return e;
            }
        }
        throw std::runtime_error("no node");
    }

    void add(uint8_t c, uint64_t id) {
        children.push_back({ c, id });
        std::sort(children.begin(),
                  children.end(),
                  [](const TestTrieElement& a, const TestTrieElement& b){
                    return a.chr < b.chr; });
    }
};

bool operator==(const TestTrieElement& lhs, const TestTrieElement& rhs) {
    return lhs.chr == rhs.chr && lhs.id == rhs.id && lhs.children == rhs.children;
}

std::ostream& operator<<(std::ostream& out, const TestTrieElement& v) {
    out << "[" << v.chr << " (" << int(v.chr) << ")|" << v.id << "]\n";
    for(auto& e: v.children) {
        std::stringstream ss;
        ss << e;
        out << indent_lines(ss.str(), 4) << "\n";
    }
    return out;
}

struct TestTrie {
    View input;
    TestTrieElement root;
};

template<typename T>
void trie_test_single(TestTrie test, bool test_values) {
    auto& should_trie = test.root;

    // Only add single \0 root for now.
    // TODO: extend this test suite to lzw-style multiple roots
    auto is_trie = TestTrieElement { '\0', 0 };
    size_t is_trie_size = 1;

    size_t remaining = test.input.size();
    auto trie = builder<T>().instance(remaining, remaining);
    trie.add_rootnode(0);

    auto is_trie_node = &is_trie;
    auto node = trie.get_rootnode(0);

    for (uint8_t c : test.input) {
        //std::cout << "char '" << char(c) << "'";
        remaining--;
        auto child = trie.find_or_insert(node, c);
        if (child.id() == lz78::undef_id) {
            //std::cout << " not found\n";
            is_trie_node->add(c,is_trie_size);

            is_trie_size++;

            is_trie_node = &is_trie;
            node = trie.get_rootnode(0);
        } else {
            //std::cout << " found\n";
            is_trie_node = &is_trie_node->find(c);
            if (test_values) {
                EXPECT_EQ(child.id(), is_trie_node->id);
            }
            node = child;
        }
    }

    ASSERT_EQ(is_trie_size, trie.size());
    ASSERT_EQ(should_trie, is_trie);
}

template<typename T>
void trie_test(bool test_values = true) {
    trie_test_single<T>({
        "abcdebcdeabc",
        {'\0',0,{
            {'a',1,{
                {'b',8,{}},
            }},
            {'b',2,{
                {'c',6,{}},
            }},
            {'c',3,{}},
            {'d',4,{
                {'e',7,{}},
            }},
            {'e',5,{}},
        }},
    }, test_values);
}

#include <tudocomp/compressors/lz78/BinaryTrie.hpp>
TEST(TrieStructure, BinaryTrie) {
    trie_test<BinaryTrie>(false);
}
TEST(Trie, BinaryTrie) {
    trie_test<BinaryTrie>();
}

/*
#include <tudocomp/compressors/lz78/HashTrie.hpp>
TEST(TrieStructure, HashTrie) {
    trie_test<HashTrie>(false);
}
TEST(Trie, HashTrie) {
    trie_test<HashTrie>();
}
*/

/*
#include <tudocomp/compressors/lz78/TernaryTrie.hpp>
TEST(TrieStructure, TernaryTrie) {
    trie_test<TernaryTrie>(false);
}
TEST(Trie, TernaryTrie) {
    trie_test<TernaryTrie>();
}
*/

#include <tudocomp/compressors/lz78/CedarTrie.hpp>
TEST(TrieStructure, CedarTrie) {
    trie_test<CedarTrie>(false);
}
TEST(Trie, CedarTrie) {
    trie_test<CedarTrie>();
}

/*
#include <tudocomp/compressors/lz78/CustomHashTrie.hpp>
TEST(TrieStructure, CustomHashTrie) {
    trie_test<CustomHashTrie>(false);
}
TEST(Trie, CustomHashTrie) {
    trie_test<CustomHashTrie>();
}
*/

/*
#include <tudocomp/compressors/lz78/MonteCarloTrie.hpp>
TEST(TrieStructure, MonteCarloTrie) {
    trie_test<MonteCarloTrie>(false);
}
TEST(Trie, MonteCarloTrie) {
    trie_test<MonteCarloTrie>();
}
*/

/*
#include <tudocomp/compressors/lz78/SecondHashTrie.hpp>
TEST(TrieStructure, SecondHashTrie) {
    trie_test<SecondHashTrie>(false);
}
TEST(Trie, SecondHashTrie) {
    trie_test<SecondHashTrie>();
}
*/

// #include <tudocomp/compressors/lz78/MBonsaiTrie.hpp>
// TEST(TrieStructure, MBonsaiGammaTrie) {
//     trie_test<MBonsaiGammaTrie>(false);
// }
// TEST(Trie, MBonsaiGammaTrie) {
//     trie_test<MBonsaiGammaTrie>();
// }
//
// TEST(TrieStructure, MBonsaiRecursiveTrie) {
//     trie_test<MBonsaiRecursiveTrie>(false);
// }
// TEST(Trie, MBonsaiRecursiveTrie) {
//     trie_test<MBonsaiRecursiveTrie>();
// }
