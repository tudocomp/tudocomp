#pragma once

//#include <tudocomp/compressors/esp/>

namespace tdc {namespace esp {
    // Implementation that covers all of 64 bit
    // TODO: This is hardcoded to work for the paper use case
    inline size_t iter_log(size_t n) {
        if (n < 7) return 0;
        if (n < 9) return 1;
        if (n < 17) return 2;
        if (n < 257) return 3;
        return 4;
    }

    inline uint64_t label(uint64_t left, uint64_t right) {
        auto diff = left ^ right;

        //std::cout << "l: " << std::setbase(2) << left << "\n";
        //std::cout << "r: " << std::setbase(2) << right << "\n";
        //std::cout << "d: " << std::setbase(2) << diff << "\n";
        //std::cout << "\n";


        DCHECK(diff != 0);

        auto l = __builtin_ctz(diff);

        auto bit = [](uint8_t l, uint64_t v) {
            // TODO: test
            return (v >> l) & 1;
        };

        // form label(A[i])
        return 2*l + bit(l, right);
    };


}}
