#pragma once

#include <tudocomp/Generator.hpp>

namespace tdc {

/// \brief Generates the k-th Thue Morse word.
///
/// Note that the k-th Thue Morse word is 2^k characters long.
class ThueMorseGenerator : public Generator {

public:
    inline static Meta meta() {
        Meta m(Generator::type_desc(), "thue_morse",
            "Generates the k-th Thue Morse word on a binary alphabet.");
        m.param("k", "The Thue Morse word order. The generated string will "
            "have length 2^k").primitive();
        return m;
    }

    inline static std::string generate(size_t n) {
        CHECK_LT(n, 64) << "too long!";

        if(n == 0) return "0";

        std::string a = "0";
        a.reserve(1ULL << n);
        for(size_t i = 1; i < n; ++i) {
            const size_t len = a.length();
            for(size_t j = 0; j < len; ++j) {
                a.push_back(a[j] == '0' ? '1' : '0');
            }
        }

        DCHECK_LE(a.length(), 1ULL << n);
        return a;
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        return generate(config().param("k").as_uint());
    }
};

} //ns

