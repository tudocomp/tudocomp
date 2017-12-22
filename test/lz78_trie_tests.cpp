#include "test/util.hpp"
#include <gtest/gtest.h>

#include <tudocomp/util.hpp>

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
    auto trie = Algorithm::instance<T>(remaining, remaining);
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
    trie_test_single<T>({
        "a",
        {'\0',0, {
            {'a',1,{}}
        }},
    }, test_values);
    trie_test_single<T>({
        "abcdefgh#defgh_abcde",
        {'\0',0, {
            {'#',9,{}},
            {'a',1,{
                {'b',13,{}},
            }},
            {'b',2,{}},
            {'c',3,{
                {'d',14,{}},
            }},
            {'d',4,{
                {'e',10,{}},
            }},
            {'e',5,{}},
            {'f',6,{
                {'g',11,{}},
            }},
            {'g',7,{}},
            {'h',8,{
                {'_',12,{}},
            }},
        }},
    }, test_values);
    trie_test_single<T>({
        "ประเทศไทย中华Việt Nam",
        {'\0',0, {
            {32,25,{}},
            {78,26,{}},
            {86,19,{}},
            {97,27,{}},
            {105,20,{}},
            {109,28,{}},
            {116,24,{}},
            {128,8,{}},
            {135,23,{}},
            {141,17,{}},
            {142,18,{}},
            {155,3,{}},
            {163,5,{}},
            {184,2,{
                {162,13,{}},
                {173,15,{}},
            }},
            {187,22,{}},
            {224,1,{
                {184,4,{
                    {151,9,{
                        {224,12,{}},
                    }},
                    {168,10,{}},
                    {176,6,{}},
                }},
                {185,7,{
                    {132,11,{}},
                }},
            }},
            {225,21,{}},
            {228,14,{}},
            {229,16,{}},
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

#include <tudocomp/compressors/lz78/BinarySortedTrie.hpp>
TEST(TrieStructure, BinarySortedTrie) {
    trie_test<BinarySortedTrie>(false);
}
TEST(Trie, BinarySortedTrie) {
    trie_test<BinarySortedTrie>();
}

#include <tudocomp/compressors/lz78/TernaryTrie.hpp>
TEST(TrieStructure, TernaryTrie) {
    trie_test<TernaryTrie>(false);
}
TEST(Trie, TernaryTrie) {
    trie_test<TernaryTrie>();
}

#include <tudocomp/compressors/lz78/CedarTrie.hpp>
TEST(TrieStructure, CedarTrie) {
    trie_test<CedarTrie>(false);
}
TEST(Trie, CedarTrie) {
    trie_test<CedarTrie>();
}

#include <tudocomp/compressors/lz78/HashTrie.hpp>
TEST(TrieStructure, HashTrie) {
    trie_test<HashTrie<>>(false);
}
TEST(Trie, HashTrie) {
    trie_test<HashTrie<>>();
}

#include <tudocomp/compressors/lz78/HashTriePlus.hpp>
TEST(TrieStructure, HashTriePlus) {
    trie_test<HashTriePlus<>>(false);
}
TEST(Trie, HashTriePlus) {
    trie_test<HashTriePlus<>>();
}

#include <tudocomp/compressors/lz78/RollingTrie.hpp>
TEST(TrieStructure, RollingTrie) {
    trie_test<RollingTrie<>>(false);
}
TEST(Trie, RollingTrie) {
    trie_test<RollingTrie<>>();
}

#include <tudocomp/compressors/lz78/RollingTriePlus.hpp>
TEST(TrieStructure, RollingTriePlus) {
    trie_test<RollingTriePlus<>>(false);
}
TEST(Trie, RollingTriePlus) {
    trie_test<RollingTriePlus<>>();
}

#include <tudocomp/compressors/lz78/ExtHashTrie.hpp>
TEST(TrieStructure, ExtHashTrie) {
    trie_test<ExtHashTrie>(false);
}
TEST(Trie, ExtHashTrie) {
    trie_test<ExtHashTrie>();
}

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
