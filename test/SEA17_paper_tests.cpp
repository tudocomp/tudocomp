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

TEST(Bwt, test) {
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
 void create_factor(int pos, int src, int len){ /* [...] */ }
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
   int m = heap.top(), fpos = sa[m], fsrc = sa[m-1], flen = heap.key(m);
   create_factor(fpos, fsrc, flen);
   for(int k=0; k < flen; k++)
    heap.remove(isa[fpos + k]);
   for(int k=0; k < flen && fpos > k; k++) {
    int s = fpos - k - 1;
    int i = isa[s];
    if(heap.contains(i)) {
     if(s + lcp[i] > fpos) {
      int l = fpos - s;
      if(l >= t)
       heap.decrease_key(i, l);
      else heap.remove(i);
}}}}}};
/////////////////////////////////
TEST(MaxHeap, test) {
    auto text_ds = builder<TextDS<>>().instance("abc\0"_v);
    auto maxheap = builder<MaxHeapStrategy<TextDS<>>>().instance();
    maxheap.factorize(text_ds, 1);
}
