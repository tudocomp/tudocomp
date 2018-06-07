#include <cctype>

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

/*
std::ostream& operator<<(std::ostream& out, const TestTrieElement& v) {
    out << "\n[" << v.chr << " (" << int(v.chr) << ")|" << v.id << "]";
    for(auto& e: v.children) {
        std::stringstream ss;
        ss << e;
        out << indent_lines(ss.str(), 4);
    }
    return out;
}
*/

std::ostream& operator<<(std::ostream& out, const TestTrieElement& v) {
    out << "\n{";
    if (std::isprint(v.chr)) {
        out << "'" << v.chr << "'";
    } else {
        out << uint32_t(uint8_t(v.chr));
    }
    out << ",";
    out << uint32_t(uint8_t(v.id));
    out << ",{";
    for(auto& e: v.children) {
        std::stringstream ss;
        ss << e;
        out << indent_lines(ss.str(), 4);
    }
    if (v.children.size() > 0) {
        out << "\n";
    }
    out << "}},";
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
        remaining--;
        auto child = trie.find_or_insert(node, c);

        // std::cout << "char '" << char(c) << "'\n";
        // trie.debug_print();

        if (child.is_new()) {
            // std::cout << " not found\n";
            is_trie_node->add(c,is_trie_size);

            // Check that insert worked correctly
            try {
                auto tmp = is_trie_node->find(c);
                ASSERT_EQ(tmp.chr, c);
                ASSERT_EQ(tmp.id, is_trie_size);
            } catch (std::runtime_error& e) {
                ASSERT_TRUE(false) << "Child node "<<c<<","<<is_trie_size<<" that should be there could not be found in the trie 2";
            }

            is_trie_size++;
            is_trie_node = &is_trie;
            node = trie.get_rootnode(0);
        } else {
            // std::cout << " found\n";

            // Check that insert worked correctly, and look at value
            try  {
                is_trie_node = &is_trie_node->find(c);
            } catch (std::runtime_error& e) {
                ASSERT_TRUE(false) << "Child node "<<c<<",? that should be there could not be found in the trie 1";
            }
            if (test_values) {
                EXPECT_EQ(child.id(), is_trie_node->id);
            }
            node = child;
        }
    }

    ASSERT_EQ(should_trie, is_trie);
    ASSERT_EQ(is_trie_size, trie.size());
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
    trie_test_single<T>({
        "0	100009425	0.1661:0.1661	#businessfo"_v,
        {0,0,{
            {9,2,{
                {'0',9,{}},
            }},
            {'#',17,{}},
            {'.',10,{}},
            {'0',1,{
                {'.',14,{}},
                {'0',4,{
                    {'9',5,{}},
                }},
            }},
            {'1',3,{
                {9,16,{}},
                {'6',11,{
                    {'6',15,{}},
                }},
                {':',13,{}},
            }},
            {'2',7,{}},
            {'4',6,{}},
            {'5',8,{}},
            {'6',12,{}},
            {'b',18,{}},
            {'e',23,{}},
            {'f',25,{}},
            {'i',21,{}},
            {'n',22,{}},
            {'o',26,{}},
            {'s',20,{
                {'s',24,{}},
            }},
            {'u',19,{}},
        }},
    }, test_values);
    /*
    trie_test_single<T>({
        "a\0\0"_v,
        {0,0,{
            {0,2,{}},
            {'a',1,{}},
        }},
    }, test_values);
    trie_test_single<T>({
        "data-web-snippets/\0\0"_v,
        {0,0,{
            {0,16,{}},
            {'-',8,{}},
            {'/',15,{}},
            {'a',2,{
                {'-',4,{}},
            }},
            {'b',7,{}},
            {'d',1,{}},
            {'e',6,{}},
            {'i',11,{}},
            {'n',10,{}},
            {'p',12,{
                {'e',13,{}},
            }},
            {'s',9,{}},
            {'t',3,{
                {'s',14,{}},
            }},
            {'w',5,{}},
        }},
    }, test_values);
    */

    // {,,{}},
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

#include <tudocomp/compressors/lz78/CompactHashTrie.hpp>
TEST(TrieStructure, CompactHashTrie) {
    trie_test<CompactHashTrie<>>(false);
}
TEST(Trie, CompactHashTrie) {
    trie_test<CompactHashTrie<>>();
}
TEST(TrieStructure, CompactHashTriePlain) {
    trie_test<CompactHashTrie<Plain>>(false);
}
TEST(Trie, CompactHashTriePlain) {
    trie_test<CompactHashTrie<Plain>>();
}
TEST(TrieStructure, CompactHashTrieSparseDisplacement) {
    trie_test<CompactHashTrie<SparseDisplacement>>(false);
}
TEST(Trie, CompactHashTrieSparseDisplacement) {
    trie_test<CompactHashTrie<SparseDisplacement>>();
}
TEST(TrieStructure, CompactHashTriePlainDisplacement) {
    trie_test<CompactHashTrie<PlainDisplacement>>(false);
}
TEST(Trie, CompactHashTriePlainDisplacement) {
    trie_test<CompactHashTrie<PlainDisplacement>>();
}
TEST(TrieStructure, CompactHashTrieSparseEliasDisplacement) {
    trie_test<CompactHashTrie<SparseEliasDisplacement>>(false);
}
TEST(Trie, CompactHashTrieSparseEliasDisplacement) {
    trie_test<CompactHashTrie<SparseEliasDisplacement>>();
}
TEST(TrieStructure, CompactHashTriePlainEliasDisplacement) {
    trie_test<CompactHashTrie<PlainEliasDisplacement>>(false);
}
TEST(Trie, CompactHashTriePlainEliasDisplacement) {
    trie_test<CompactHashTrie<PlainEliasDisplacement>>();
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
