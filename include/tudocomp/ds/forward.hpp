#pragma once

namespace tdc {

template<typename T>
class SuffixArray;
template<typename T>
class InverseSuffixArray;
template<typename T>
class PhiArray;

struct LCPArray;
template<typename select_t = sdsl::select_support_mcl<1,1>> struct LCPSada;

template<class lcp_t, template<typename> class isa_t>
class TextDS;


}//ns

