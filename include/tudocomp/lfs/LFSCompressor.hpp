#ifndef _INCLUDED_LFS_COMPRESSOR_HPP_
#define _INCLUDED_LFS_COMPRESSOR_HPP_

#include <tudocomp/tudocomp.hpp>

namespace tdc {

class LFSCompressor : public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "longest first substitution",
            "This is an implementation of the longest first substitution compression scheme.");
        return m;
    }

    inline LFSCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
    }

    inline virtual void decompress(Input& input, Output& output) override {
    }

};

}


#endif
