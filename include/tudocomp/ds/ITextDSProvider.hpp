#ifndef _INCLUDED_DS_IPROVIDER_HPP
#define _INCLUDED_DS_IPROVIDER_HPP

namespace tudocomp {

//fwd
class SuffixArray;
class InverseSuffixArray;
class PhiArray;
class LCPArray;

class ITextDSProvider {

public:
    typedef void (*jit_init_t)(void*, size_t);
    typedef void (*jit_t)(void*, size_t, size_t);

    virtual const SuffixArray& require_sa() = 0;
    virtual const InverseSuffixArray& require_isa() = 0;
    virtual const PhiArray& require_phi() = 0;
    virtual const LCPArray& require_lcp() = 0;

    virtual uint8_t operator[](size_t) const = 0;
    virtual const uint8_t* text() const = 0;
    virtual size_t size() const = 0;
};

}

#endif

