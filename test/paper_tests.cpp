#include "gtest/gtest.h"
#include "test/util.hpp"

//m.option("textds").templated<text_t, TextDS<>>();
//template<typename text_t>

/////////////////////////////////
#include <tudocomp/tudocomp.hpp>
class BWTComp : public Compressor {
  public: static Meta meta() {
    Meta m("compressor", "bwt");
    m.option("ds").templated<TextDS<>>();
    m.needs_sentinel_terminator();
    return m; }
  using Compressor::Compressor;
  void compress(Input& in, Output& out) {
    auto o = out.as_stream();
    auto i = in.as_view();
    TextDS<> t(env().env_for_option("ds"),i);
    const auto& sa = t.require_sa();
    for(size_t j = 0; j < t.size(); ++j)
      o << ((sa[j] != 0) ? t[sa[j] - 1]
                         : t[t.size() - 1]);
  }
  void decompress(Input&, Output&){/*[...]*/}
};
/////////////////////////////////
TEST(SEA17, Bwt) {
    auto i = test::compress_input("aaababaaabaababa");
    auto o = test::compress_output();
    auto c = tdc::builder<BWTComp>().instance();
    c.compress(i, o);
    ASSERT_EQ(o.result(), "abb\0ababbaaaaaaaa"_v);
}

/////////////////////////////////
template<class text_t>
class MaxHeapStrategy : public Algorithm {
 public: static Meta meta() {
  Meta m("lcpcomp_strategy", "heap");
  return m; }
 using Algorithm::Algorithm;
 void create_factor(int pos,int src,int len);
 void factorize(text_t& text, const int t) {
  text.require(text_t::SA | text_t::ISA | text_t::LCP);
  auto& sa = text.require_sa();
  auto& isa = text.require_isa();
  auto lcpp = text.release_lcp()->relinquish();
  auto& lcp = *lcpp;
  ArrayMaxHeap<typename text_t::lcp_type::data_type> heap(lcp, lcp.size(), lcp.size());
  for(int i = 1; i < lcp.size(); ++i)
   if(lcp[i] >= t) heap.insert(i);
  while(heap.size() > 0) {
   int i = heap.top(), fpos = sa[i],
       fsrc = sa[i-1], flen = heap.key(i);
   create_factor(fpos, fsrc, flen);
   for(int k=0; k < flen; k++)
    heap.remove(isa[fpos + k]);
   for(int k=0; k < flen && fpos > k; k++) {
    int s = fpos - k - 1;
    int j = isa[s];
    if(heap.contains(j)) {
     if(s + lcp[j] > fpos) {
      int l = fpos - s;
      if(l >= t)
       heap.decrease_key(j, l);
      else heap.remove(j);
}}}}}};
/////////////////////////////////
template<class T>
void MaxHeapStrategy<T>::create_factor(int pos, int src, int len){ /* [...] */ }

TEST(SEA17, MaxHeap) {
    auto text_ds = builder<TextDS<>>().instance("abc\0"_v);
    auto maxheap = builder<MaxHeapStrategy<TextDS<>>>().instance();
    maxheap.factorize(text_ds, 1);
}
/////////////////////////////////
void factorize(TextDS<>& T, SuffixTree& ST, std::function<void(int begin, int end, int ref)> output){
 typedef SuffixTree::node_type node_t;
 sdsl::int_vector<> R(ST.internal_nodes,0,bits_for(T.size() * bits_for(ST.cst.csa.sigma) / bits_for(T.size())));
 int pos = 0, z = 0;
 while(pos < T.size() - 1) {
  node_t l = ST.select_leaf(ST.cst.csa.isa[pos]);
  int leaflabel = pos;
  if(ST.parent(l) == ST.root || R[ST.nid(ST.parent(l))] != 0) {
   int parent_strdepth = ST.str_depth(ST.parent(l));
   output(pos + parent_strdepth, pos + parent_strdepth + 1, R[ST.nid(ST.parent(l))]);
   pos += parent_strdepth+1;
   ++z;
   continue;
  }
  int d = 1;
  node_t parent = ST.root;
  node_t node = ST.level_anc(l, d);
  while(R[ST.nid(node)] != 0) {
   parent = node;
   node = ST.level_anc(l, ++d);
  }
  pos += ST.str_depth(parent);
  int begin = leaflabel + ST.str_depth(parent);
  int end = leaflabel + ST.str_depth(node);
  output(begin, end, R[ST.nid(ST.parent(node))]);
  R[ST.nid(node)] = ++z;
  pos += end - begin;
 }
}
/////////////////////////////////
TEST(SEA17, factorize) {
    auto T = "aaababaaabaababa\0"_v;
    auto text_ds = builder<TextDS<>>().instance(T);
    std::vector<int> v;
    auto f = [&](int begin, int end, int ref) {
        v.push_back(begin);
        v.push_back(end);
        v.push_back(ref);
    };
    SuffixTree::cst_t backing_cst;
    {
        std::string bad_copy_1 = T.slice(0, T.size() - 1);
        construct_im(backing_cst, bad_copy_1, 1);
    }
    SuffixTree ST(backing_cst);

    factorize(text_ds, ST,f);

    ASSERT_EQ(v, (std::vector<int> {
        0,   1,   0,
        2,   3,   1,
        3,   5,   0,
        7,   8,   3,
        9,  11,   1,
        14, 16,   5,
        // 16, 17,   0,
    }));
}
