#ifndef FORWARD_HH
#define FORWARD_HH
namespace tdc {

template<typename T>
class SuffixArray;
template<typename T>
class InverseSuffixArray;
template<typename T>
class PhiArray;
template<typename T, typename sa_t, typename select_t>
class lcp_sada;
template<typename T>
class LCPArray;

template<template<typename> class lcp_t, template<typename> class isa_t>
class TextDS;


}//ns
#endif /* FORWARD_HH */
