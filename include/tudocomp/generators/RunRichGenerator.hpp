#pragma once

#include <tudocomp/Generator.hpp>

namespace tdc {

/// \brief Generates strings according to A Series of Run-Rich Strings
/// (Wataru Matsubara et al.)
class RunRichGenerator : public Generator {

public:
    inline static Meta meta() {
        Meta m(Generator::type_desc(), "run_rich",
            "Generates a run-rich string on a binary alphabet conforming "
            "[Matsubara et al., 2009].");
        m.param("n", "The length of the generated string.").primitive();
        return m;
    }

    inline static std::string generate(size_t n) {
        std::string t0 = "0110101101001011010",
            t1 = "0110101101001",
            t2 = "01101011010010110101101",
            t3 = t2+t1;

        if(n==0) return t0;
        if(n==1) return t1;
        if(n==2) return t2;

        for(size_t i = 4; i < n; ++i) {
            std::string tmp = (i % 3 == 0) ? (t3+t2) : (t3+t0);
            t0=t1;
            t1=t2;
            t2=t3;
            t3=tmp;
        }

        return t3;
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        return generate(config().param("n").as_uint());
    }
};

} //ns

